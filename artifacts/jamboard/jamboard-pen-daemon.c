/*
 * jamboard-pen-daemon.c — Google Jamboard (baracus) RAPT Optical Stylus/Touch Daemon
 *
 * Clean-room Linux/Android user-space driver for the Google Jamboard's RAPT Touch
 * ("HazelRed" / hazeldos, VID:PID 1cbe:0003 or 2986:0137) optical InGlass bezel.
 *
 * Why this daemon is needed:
 *   1. The RAPT controller streams 340-byte HID Input Reports (Report ID 0xF1)
 *      containing 24 x 14-byte contact slots. Each slot carries X, Y, Width,
 *      Height, Tip Pressure, and a custom 16-bit field (Vendor 0xFF00:0x0001) at
 *      bytes +12..+13, all wrapped inside Logical Collection (Finger).
 *   2. Unmodified Linux hid-multitouch ignores 0xFF00:0x0001, hardcodes every
 *      contact to MT_TOOL_FINGER, and exposes duplicate evdev nodes (input0/input1).
 *   3. This daemon grabs the raw kernel evdev nodes (EVIOCGRAB), parses Report
 *      0xF1 directly from /dev/hidraw*, and exposes two clean uinput devices:
 *        - "Jamboard RAPT Finger Touch" (24-slot Type-B Direct Multi-Touch)
 *        - "Jamboard RAPT Stylus & Eraser" (Direct Pen/Rubber Tablet with
 *          BTN_TOOL_PEN, BTN_TOOL_RUBBER, optical wedge ABS_PRESSURE +
 *          synthesized ABS_TILT_X / ABS_TILT_Y, open-ended universal eraser
 *          sizing, and spatial palm/whiteboard-block contact clustering)
 *
 * Build (Fedora aarch64):
 *   gcc -O2 -Wall -Wextra -o jamboard-pen-daemon jamboard-pen-daemon.c -lm
 *
 * Cross-Build Static Binary (for live LineageOS ADB testing):
 *   aarch64-linux-gnu-gcc -O2 -static -Wall -Wextra -o jamboard-pen-daemon jamboard-pen-daemon.c -lm
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define RAPT_VID_GOOGLE       0x1cbe
#define RAPT_PID_GOOGLE       0x0003
#define RAPT_VID_GENERIC      0x2986
#define RAPT_PID_GENERIC      0x0137

#define RAPT_REPORT_ID        0xF1
#define RAPT_REPORT_LEN       340
#define RAPT_MAX_SLOTS        24
#define RAPT_RDESC_TARGET_LEN 2313

/* Physical active area from rapt_hid_descriptor.bin: 1210.0 mm x 680.0 mm */
#define RAPT_LOGICAL_MAX_XY   29999
#define RAPT_LOGICAL_MAX_WH   14999
#define RAPT_LOGICAL_MAX_P    29999
#define RAPT_PHYS_WIDTH_MM    1210.0f
#define RAPT_PHYS_HEIGHT_MM   680.0f
#define RAPT_RES_X_UNITS_MM   25  /* ~29999 / 1210 mm */
#define RAPT_RES_Y_UNITS_MM   44  /* ~29999 / 680 mm  */

#define MAX_GRABBED_EVDEV     8

enum tool_kind {
    TOOL_NONE   = 0,
    TOOL_FINGER = 1,
    TOOL_STYLUS = 2,
    TOOL_ERASER = 3,
};

struct daemon_cfg {
    const char *hidraw_path;
    bool grab_evdev;
    bool dump_frames;
    bool verbose;
    bool geometry_fallback;
    bool use_norm_bands;
    int vendor_stylus;
    int vendor_eraser;
    int vendor_eraser_alt;
    float stylus_max_minor_mm;
    float wedge_start_mm;
    float wedge_full_mm;
    float eraser_min_mm;
    float eraser_cluster_mm;
    float norm_stylus_max;
    float norm_eraser_min;
    float latch_radius_mm;
    int latch_ms;
};

struct contact_raw {
    bool active;
    uint8_t cid;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t pressure;
    uint16_t vendor01;
    float x_mm;
    float y_mm;
    float w_mm;
    float h_mm;
    float major_mm;
    float minor_mm;
    float norm_size;
};

struct slot_tracker {
    bool active;
    uint8_t cid;
    enum tool_kind kind;
    int tracking_id;
    bool sent_as_finger;
    bool absorbed_by_eraser;
};

struct tool_latch {
    enum tool_kind kind;
    float x_mm;
    float y_mm;
    int64_t last_seen_ms;
};

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int sig) {
    (void)sig;
    g_stop = 1;
}

static int64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static bool is_rapt_vid_pid(int vendor, int product) {
    return (vendor == RAPT_VID_GOOGLE && product == RAPT_PID_GOOGLE) ||
           (vendor == RAPT_VID_GENERIC && product == RAPT_PID_GENERIC);
}

static int emit_event(int fd, uint16_t type, uint16_t code, int32_t value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return write(fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev) ? 0 : -1;
}

static int find_rapt_hidraw(char *out_path, size_t out_len) {
    DIR *dir = opendir("/dev");
    if (!dir) return -1;

    char best_fallback[128] = {0};
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strncmp(de->d_name, "hidraw", 6) != 0) continue;

        char cand[128];
        snprintf(cand, sizeof(cand), "/dev/%s", de->d_name);
        int fd = open(cand, O_RDWR | O_NONBLOCK);
        if (fd < 0) fd = open(cand, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct hidraw_devinfo info;
        int rdesc_size = 0;
        if (ioctl(fd, HIDIOCGRAWINFO, &info) == 0 &&
            is_rapt_vid_pid(info.vendor & 0xFFFF, info.product & 0xFFFF)) {
            if (ioctl(fd, HIDIOCGRDESCSIZE, &rdesc_size) == 0 &&
                rdesc_size == RAPT_RDESC_TARGET_LEN) {
                snprintf(out_path, out_len, "%s", cand);
                close(fd);
                closedir(dir);
                return 0;
            }
            if (best_fallback[0] == '\0' && rdesc_size >= 500) {
                snprintf(best_fallback, sizeof(best_fallback), "%s", cand);
            }
        }
        close(fd);
    }
    closedir(dir);

    if (best_fallback[0] != '\0') {
        snprintf(out_path, out_len, "%s", best_fallback);
        return 0;
    }
    return -1;
}

static size_t grab_rapt_evdev_nodes(int *grab_fds, size_t max_fds, bool verbose) {
    DIR *dir = opendir("/dev/input");
    if (!dir) return 0;

    size_t count = 0;
    struct dirent *de;
    while ((de = readdir(dir)) != NULL && count < max_fds) {
        if (strncmp(de->d_name, "event", 5) != 0) continue;

        char path[128];
        snprintf(path, sizeof(path), "/dev/input/%s", de->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct input_id id;
        char name[128] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
        if (ioctl(fd, EVIOCGID, &id) == 0 &&
            is_rapt_vid_pid(id.vendor, id.product) &&
            strstr(name, "Jamboard RAPT") == NULL) {
            if (ioctl(fd, EVIOCGRAB, 1) == 0) {
                grab_fds[count++] = fd;
                if (verbose) {
                    fprintf(stderr, "[jamboard-pen] Grabbed raw evdev %s (%s)\n", path, name);
                }
                continue;
            }
        }
        close(fd);
    }
    closedir(dir);
    return count;
}

static void setup_abs(int ufd, uint16_t code, int32_t min, int32_t max, int32_t res) {
    struct uinput_abs_setup abs;
    memset(&abs, 0, sizeof(abs));
    abs.code = code;
    abs.absinfo.minimum = min;
    abs.absinfo.maximum = max;
    abs.absinfo.resolution = res;
    if (ioctl(ufd, UI_ABS_SETUP, &abs) < 0) {
        perror("UI_ABS_SETUP");
    }
}

static int create_finger_uinput(void) {
    int ufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ufd < 0) return -1;

    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(ufd, UI_SET_EVBIT, EV_ABS);
    ioctl(ufd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    setup_abs(ufd, ABS_X, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_X_UNITS_MM);
    setup_abs(ufd, ABS_Y, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_Y_UNITS_MM);
    setup_abs(ufd, ABS_MT_SLOT, 0, RAPT_MAX_SLOTS - 1, 0);
    setup_abs(ufd, ABS_MT_TRACKING_ID, 0, 65535, 0);
    setup_abs(ufd, ABS_MT_POSITION_X, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_X_UNITS_MM);
    setup_abs(ufd, ABS_MT_POSITION_Y, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_Y_UNITS_MM);
    setup_abs(ufd, ABS_MT_TOUCH_MAJOR, 0, RAPT_LOGICAL_MAX_WH, RAPT_RES_X_UNITS_MM);
    setup_abs(ufd, ABS_MT_TOUCH_MINOR, 0, RAPT_LOGICAL_MAX_WH, RAPT_RES_X_UNITS_MM);
    setup_abs(ufd, ABS_MT_ORIENTATION, -90, 90, 0);
    setup_abs(ufd, ABS_MT_PRESSURE, 0, RAPT_LOGICAL_MAX_P, 0);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor  = 0x18d1;
    usetup.id.product = 0x4e01;
    usetup.id.version = 1;
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "Jamboard RAPT Finger Touch");

    if (ioctl(ufd, UI_DEV_SETUP, &usetup) < 0 || ioctl(ufd, UI_DEV_CREATE) < 0) {
        close(ufd);
        return -1;
    }
    return ufd;
}

static int create_pen_uinput(void) {
    int ufd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ufd < 0) return -1;

    ioctl(ufd, UI_SET_EVBIT, EV_KEY);
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOOL_PEN);
    ioctl(ufd, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
    ioctl(ufd, UI_SET_EVBIT, EV_ABS);
    ioctl(ufd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    setup_abs(ufd, ABS_X, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_X_UNITS_MM);
    setup_abs(ufd, ABS_Y, 0, RAPT_LOGICAL_MAX_XY, RAPT_RES_Y_UNITS_MM);
    setup_abs(ufd, ABS_PRESSURE, 0, 4095, 0);
    setup_abs(ufd, ABS_TILT_X, -90, 90, 0);
    setup_abs(ufd, ABS_TILT_Y, -90, 90, 0);

    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor  = 0x18d1;
    usetup.id.product = 0x4e02;
    usetup.id.version = 1;
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "Jamboard RAPT Stylus & Eraser");

    if (ioctl(ufd, UI_DEV_SETUP, &usetup) < 0 || ioctl(ufd, UI_DEV_CREATE) < 0) {
        close(ufd);
        return -1;
    }
    return ufd;
}

static bool is_eraser_sized(const struct contact_raw *c, const struct daemon_cfg *cfg) {
    if (cfg->use_norm_bands) {
        /* Stock JamKiosk DrawingController::TouchTypeFromSize(float) @ 0x00474714:
         * open-ended [0.475f, +inf) matches puck, any whiteboard block, or palm heel */
        return c->norm_size >= cfg->norm_eraser_min;
    }
    /* Open-ended physical threshold [eraser_min_mm, +inf) — no upper cap */
    return c->major_mm >= cfg->eraser_min_mm;
}

static enum tool_kind classify_new_contact(const struct contact_raw *c,
                                           const struct daemon_cfg *cfg,
                                           const struct tool_latch *latch,
                                           int64_t ts_ms) {
    /* 1. Spatial/temporal bounce latch (mirrors stock mEraserLatchRange / mEraserDelayTime) */
    if (latch->kind != TOOL_NONE && (ts_ms - latch->last_seen_ms) <= cfg->latch_ms) {
        float dx = c->x_mm - latch->x_mm;
        float dy = c->y_mm - latch->y_mm;
        if (hypotf(dx, dy) <= cfg->latch_radius_mm) {
            return latch->kind;
        }
    }

    /* 2. Hardware RAPT Vendor 0xFF00:0x0001 classification */
    if (c->vendor01 != 0) {
        if ((int)c->vendor01 == cfg->vendor_eraser ||
            (cfg->vendor_eraser_alt >= 0 && (int)c->vendor01 == cfg->vendor_eraser_alt)) {
            return TOOL_ERASER;
        }
        if ((int)c->vendor01 == cfg->vendor_stylus) {
            return TOOL_STYLUS;
        }
    }

    /* 3. Open-ended Universal Eraser & Stylus size bands */
    if (is_eraser_sized(c, cfg)) {
        return TOOL_ERASER;
    }
    if (cfg->use_norm_bands) {
        if (c->norm_size > 0.0f && c->norm_size < cfg->norm_stylus_max) {
            return TOOL_STYLUS;
        }
        return TOOL_FINGER;
    }
    if (cfg->geometry_fallback && c->minor_mm > 0.1f && c->minor_mm <= cfg->stylus_max_minor_mm) {
        return TOOL_STYLUS;
    }

    return TOOL_FINGER;
}

static int32_t compute_eraser_pressure(float span_mm, float min_mm) {
    /* Map eraser diameter / palm cluster span [min_mm .. 180mm] -> [1200 .. 4095]
     * so pressure-sensitive whiteboard apps widen the eraser swath for palms & blocks */
    float denom = 180.0f - min_mm;
    if (denom < 10.0f) denom = 10.0f;
    float t = (span_mm - min_mm) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return 1200 + (int32_t)lrintf(t * 2895.0f);
}

static void compute_stylus_tilt(const struct contact_raw *c,
                                const struct daemon_cfg *cfg,
                                int32_t *tilt_x,
                                int32_t *tilt_y) {
    *tilt_x = 0;
    *tilt_y = 0;

    /* Below wedge_start_mm (~4.7 mm = 15 px on 4K), stylus is upright fine-tip */
    if (c->major_mm <= cfg->wedge_start_mm) {
        return;
    }

    float t = (c->major_mm - cfg->wedge_start_mm) /
              (cfg->wedge_full_mm - cfg->wedge_start_mm);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float deg = t * 70.0f; /* 0..70 degrees tilt shading */
    float sum = c->w_mm + c->h_mm;
    if (sum < 0.1f) sum = 0.1f;

    *tilt_x = (int32_t)lrintf(deg * (c->w_mm / sum));
    *tilt_y = (int32_t)lrintf(deg * (c->h_mm / sum));
}

static uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --hidraw <path>            Explicit /dev/hidrawX path (auto-detected by default)\n"
        "  --no-grab                  Do not EVIOCGRAB raw RAPT kernel evdev nodes\n"
        "  --dump                     Print decoded active contacts to stdout\n"
        "  --verbose                  Verbose startup and tool transition logging\n"
        "  --no-geometry-fallback     Classify stylus strictly from Vendor 0xFF00:0x0001\n"
        "  --use-norm-bands           Use stock JamKiosk normalized avg(W,H)/14999 bands\n"
        "                             (<0.40 stylus, 0.40..0.475 finger, >=0.475 eraser)\n"
        "  --vendor-stylus <int>      Vendor 0xFF00:0x0001 value for Stylus (default: 1)\n"
        "  --vendor-eraser <int>      Vendor 0xFF00:0x0001 value for Eraser (default: 3)\n"
        "  --stylus-minor-mm <float>  Max minor-axis mm for passive stylus (default: 4.6)\n"
        "  --eraser-min-mm <float>    Min major-axis mm for any eraser/palm (default: 32.0)\n"
        "  --eraser-cluster-mm <flt>  Capture radius mm to merge palm/block blobs (default: 70.0)\n"
        "  -h, --help                 Show this help\n",
        prog);
}

int main(int argc, char **argv) {
    struct daemon_cfg cfg = {
        .hidraw_path         = NULL,
        .grab_evdev          = true,
        .dump_frames         = false,
        .verbose             = false,
        .geometry_fallback   = true,
        .use_norm_bands      = false,
        .vendor_stylus       = 1,
        .vendor_eraser       = 3,
        .vendor_eraser_alt   = -1,
        .stylus_max_minor_mm = 4.6f,
        .wedge_start_mm      = 4.7f,
        .wedge_full_mm       = 18.5f,
        .eraser_min_mm       = 32.0f,
        .eraser_cluster_mm   = 70.0f,
        .norm_stylus_max     = 0.40f,
        .norm_eraser_min     = 0.475f,
        .latch_radius_mm     = 55.0f,
        .latch_ms            = 150,
    };

    static const struct option long_opts[] = {
        {"hidraw",               required_argument, NULL, 'd'},
        {"no-grab",              no_argument,       NULL, 'G'},
        {"dump",                 no_argument,       NULL, 'D'},
        {"verbose",              no_argument,       NULL, 'v'},
        {"no-geometry-fallback", no_argument,       NULL, 'F'},
        {"use-norm-bands",       no_argument,       NULL, 'N'},
        {"vendor-stylus",        required_argument, NULL, 's'},
        {"vendor-eraser",        required_argument, NULL, 'e'},
        {"stylus-minor-mm",      required_argument, NULL, 'm'},
        {"eraser-min-mm",        required_argument, NULL, 'E'},
        {"eraser-cluster-mm",    required_argument, NULL, 'C'},
        {"help",                 no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:GDvFNs:e:m:E:C:h", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': cfg.hidraw_path = optarg; break;
            case 'G': cfg.grab_evdev = false; break;
            case 'D': cfg.dump_frames = true; break;
            case 'v': cfg.verbose = true; break;
            case 'F': cfg.geometry_fallback = false; break;
            case 'N': cfg.use_norm_bands = true; break;
            case 's': cfg.vendor_stylus = atoi(optarg); break;
            case 'e': cfg.vendor_eraser = atoi(optarg); break;
            case 'm': cfg.stylus_max_minor_mm = strtof(optarg, NULL); break;
            case 'E': cfg.eraser_min_mm = strtof(optarg, NULL); break;
            case 'C': cfg.eraser_cluster_mm = strtof(optarg, NULL); break;
            case 'h':
            default:
                usage(argv[0]);
                return opt == 'h' ? 0 : 1;
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    char auto_hidraw[128] = {0};
    if (!cfg.hidraw_path) {
        if (find_rapt_hidraw(auto_hidraw, sizeof(auto_hidraw)) < 0) {
            fprintf(stderr, "[jamboard-pen] ERROR: RAPT hidraw device not found.\n");
            return 1;
        }
        cfg.hidraw_path = auto_hidraw;
    }

    int hid_fd = open(cfg.hidraw_path, O_RDWR);
    if (hid_fd < 0) hid_fd = open(cfg.hidraw_path, O_RDONLY);
    if (hid_fd < 0) {
        perror("[jamboard-pen] open hidraw");
        return 1;
    }

    /* Ensure RAPT max contact count (Feature Report 0xF2 = 24) is enabled */
    uint8_t feat_max_contacts[2] = {0xF2, 0x18};
    ioctl(hid_fd, HIDIOCSFEATURE(sizeof(feat_max_contacts)), feat_max_contacts);

    int grab_fds[MAX_GRABBED_EVDEV];
    size_t num_grabbed = 0;
    if (cfg.grab_evdev) {
        num_grabbed = grab_rapt_evdev_nodes(grab_fds, MAX_GRABBED_EVDEV, cfg.verbose);
    }

    int finger_ufd = create_finger_uinput();
    int pen_ufd    = create_pen_uinput();
    if (finger_ufd < 0 || pen_ufd < 0) {
        perror("[jamboard-pen] create uinput (is /dev/uinput accessible?)");
        return 1;
    }

    fprintf(stderr,
            "[jamboard-pen] Active on %s (grabbed %zu raw evdev node(s)). Streaming to uinput.\n",
            cfg.hidraw_path, num_grabbed);

    struct slot_tracker slots[RAPT_MAX_SLOTS];
    memset(slots, 0, sizeof(slots));
    struct tool_latch latch = {0};
    int next_tracking_id = 1;
    enum tool_kind prev_pen_tool = TOOL_NONE;

    uint8_t buf[512];
    while (!g_stop) {
        ssize_t r = read(hid_fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("[jamboard-pen] hidraw read");
            break;
        }
        if (r < RAPT_REPORT_LEN || buf[0] != RAPT_REPORT_ID) {
            continue;
        }

        int64_t ts = now_ms();
        uint8_t contact_count = buf[337];
        uint16_t scan_time = get_u16_le(&buf[338]);

        struct contact_raw contacts[RAPT_MAX_SLOTS];
        int seed_eraser_slot = -1;
        float seed_eraser_major = -1.0f;

        /* Pass 1: Decode all 24 slots, retire lifted contacts, and classify tools */
        for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
            const uint8_t *s = &buf[1 + i * 14];
            struct contact_raw *c = &contacts[i];
            c->active    = (s[0] & 0x01) != 0;
            c->cid       = s[1];
            c->x         = get_u16_le(&s[2]);
            c->y         = get_u16_le(&s[4]);
            c->w         = get_u16_le(&s[6]);
            c->h         = get_u16_le(&s[8]);
            c->pressure  = get_u16_le(&s[10]);
            c->vendor01  = get_u16_le(&s[12]);

            c->x_mm      = (float)c->x * RAPT_PHYS_WIDTH_MM / 30000.0f;
            c->y_mm      = (float)c->y * RAPT_PHYS_HEIGHT_MM / 30000.0f;
            c->w_mm      = (float)c->w * RAPT_PHYS_WIDTH_MM / 30000.0f;
            c->h_mm      = (float)c->h * RAPT_PHYS_HEIGHT_MM / 30000.0f;
            c->major_mm  = c->w_mm >= c->h_mm ? c->w_mm : c->h_mm;
            c->minor_mm  = c->w_mm >= c->h_mm ? c->h_mm : c->w_mm;
            c->norm_size = 0.5f * (float)(c->w + c->h) / (float)RAPT_LOGICAL_MAX_WH;

            if (!c->active) {
                if (slots[i].active) {
                    if (slots[i].sent_as_finger) {
                        emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                        emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                    }
                    slots[i].active             = false;
                    slots[i].kind               = TOOL_NONE;
                    slots[i].sent_as_finger     = false;
                    slots[i].absorbed_by_eraser = false;
                }
                continue;
            }

            if (!slots[i].active || slots[i].cid != c->cid) {
                if (slots[i].active && slots[i].sent_as_finger) {
                    emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                    emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                }
                slots[i].active             = true;
                slots[i].cid                = c->cid;
                slots[i].tracking_id        = next_tracking_id++ & 0x7FFF;
                slots[i].sent_as_finger     = false;
                slots[i].absorbed_by_eraser = false;
                slots[i].kind               = classify_new_contact(c, &cfg, &latch, ts);
                if (cfg.verbose && slots[i].kind != TOOL_FINGER) {
                    fprintf(stderr,
                            "[jamboard-pen] Slot %d cid=%u -> %s (major=%.1fmm minor=%.1fmm norm=%.3f P=%u v01=%u)\n",
                            i, c->cid,
                            slots[i].kind == TOOL_ERASER ? "ERASER" : "STYLUS",
                            c->major_mm, c->minor_mm, c->norm_size, c->pressure, c->vendor01);
                }
            } else if (slots[i].kind != TOOL_ERASER && is_eraser_sized(c, &cfg)) {
                /* Promote to eraser if palm heel or whiteboard block settles flat on frame 2+ */
                slots[i].kind = TOOL_ERASER;
                if (slots[i].sent_as_finger) {
                    emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                    emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                    slots[i].sent_as_finger = false;
                }
                if (cfg.verbose) {
                    fprintf(stderr,
                            "[jamboard-pen] Slot %d cid=%u promoted -> ERASER (major=%.1fmm norm=%.3f)\n",
                            i, c->cid, c->major_mm, c->norm_size);
                }
            }

            if (slots[i].kind == TOOL_ERASER && c->major_mm > seed_eraser_major) {
                seed_eraser_slot  = i;
                seed_eraser_major = c->major_mm;
            }
        }

        /* Pass 2: Spatial Eraser Clustering (mirrors JamKiosk UpdateEraserTouchGroups @ 0x00474754)
         * Absorb nearby palm/knuckle/felt-block ridges within eraser_cluster_mm (~70 mm) into a
         * single cohesive eraser TouchGroup so stray blobs never pan/zoom the canvas. */
        bool in_eraser_cluster[RAPT_MAX_SLOTS] = {false};
        float eraser_x_raw = 0.0f, eraser_y_raw = 0.0f;
        float eraser_cx_mm = 0.0f, eraser_cy_mm = 0.0f;
        float eraser_span_mm = 0.0f;

        if (seed_eraser_slot >= 0) {
            float seed_x_mm = contacts[seed_eraser_slot].x_mm;
            float seed_y_mm = contacts[seed_eraser_slot].y_mm;
            float w_sum = 0.0f;

            for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
                if (!contacts[i].active) continue;
                float dist = hypotf(contacts[i].x_mm - seed_x_mm, contacts[i].y_mm - seed_y_mm);
                if (slots[i].kind == TOOL_ERASER ||
                    slots[i].absorbed_by_eraser ||
                    dist <= cfg.eraser_cluster_mm) {
                    in_eraser_cluster[i] = true;
                    slots[i].absorbed_by_eraser = true;
                    if (slots[i].sent_as_finger) {
                        emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                        emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                        slots[i].sent_as_finger = false;
                    }
                    float weight = contacts[i].major_mm > 5.0f ? contacts[i].major_mm : 5.0f;
                    eraser_x_raw += (float)contacts[i].x * weight;
                    eraser_y_raw += (float)contacts[i].y * weight;
                    eraser_cx_mm += contacts[i].x_mm * weight;
                    eraser_cy_mm += contacts[i].y_mm * weight;
                    w_sum        += weight;
                }
            }

            if (w_sum > 0.0f) {
                eraser_x_raw /= w_sum;
                eraser_y_raw /= w_sum;
                eraser_cx_mm /= w_sum;
                eraser_cy_mm /= w_sum;
            }

            /* Compute overall swath span (mirrors TouchGroup::GetRadius @ 0x00476e04) */
            float max_rad_mm = 0.0f;
            for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
                if (!in_eraser_cluster[i]) continue;
                float r_mm = hypotf(contacts[i].x_mm - eraser_cx_mm,
                                    contacts[i].y_mm - eraser_cy_mm) +
                             0.5f * contacts[i].major_mm;
                if (r_mm > max_rad_mm) max_rad_mm = r_mm;
            }
            eraser_span_mm = 2.0f * max_rad_mm;
        }

        int primary_stylus_slot = -1;
        int active_fingers = 0;
        int first_finger_slot = -1;

        for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
            const struct contact_raw *c = &contacts[i];
            if (!c->active) continue;

            if (cfg.dump_frames) {
                printf("scan=%5u cnt=%2u slot=%2d cid=%3u X=%5u Y=%5u W=%4u(%.1fmm) H=%4u(%.1fmm) norm=%.3f P=%5u v01=%u -> %s\n",
                       scan_time, contact_count, i, c->cid, c->x, c->y,
                       c->w, c->w_mm, c->h, c->h_mm, c->norm_size, c->pressure, c->vendor01,
                       in_eraser_cluster[i] ? "ERASER_CLUSTER" :
                       (slots[i].absorbed_by_eraser ? "ERASER_TRAIL" :
                       (slots[i].kind == TOOL_STYLUS ? "STYLUS" : "FINGER")));
            }

            /* Suppress trailing knuckles that were part of an eraser wipe until lift-off */
            if (in_eraser_cluster[i] || slots[i].absorbed_by_eraser) {
                continue;
            }

            if (slots[i].kind == TOOL_STYLUS) {
                if (primary_stylus_slot < 0) primary_stylus_slot = i;
            } else {
                active_fingers++;
                if (first_finger_slot < 0) first_finger_slot = i;
                slots[i].sent_as_finger = true;

                uint16_t maj_raw = c->w >= c->h ? c->w : c->h;
                uint16_t min_raw = c->w >= c->h ? c->h : c->w;
                int32_t orient   = c->w >= c->h ? 90 : 0;

                emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, slots[i].tracking_id);
                emit_event(finger_ufd, EV_ABS, ABS_MT_POSITION_X, c->x);
                emit_event(finger_ufd, EV_ABS, ABS_MT_POSITION_Y, c->y);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, maj_raw);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TOUCH_MINOR, min_raw);
                emit_event(finger_ufd, EV_ABS, ABS_MT_ORIENTATION, orient);
                emit_event(finger_ufd, EV_ABS, ABS_MT_PRESSURE, c->pressure);
            }
        }

        /* Sync Finger multi-touch uinput device */
        emit_event(finger_ufd, EV_KEY, BTN_TOUCH, active_fingers > 0 ? 1 : 0);
        if (first_finger_slot >= 0) {
            emit_event(finger_ufd, EV_ABS, ABS_X, contacts[first_finger_slot].x);
            emit_event(finger_ufd, EV_ABS, ABS_Y, contacts[first_finger_slot].y);
        }
        emit_event(finger_ufd, EV_SYN, SYN_REPORT, 0);

        /* Sync Stylus / Universal Eraser direct tablet uinput device */
        if (seed_eraser_slot >= 0 || primary_stylus_slot >= 0) {
            enum tool_kind cur_tool = seed_eraser_slot >= 0 ? TOOL_ERASER : TOOL_STYLUS;
            int32_t out_x, out_y, scaled_p, tilt_x = 0, tilt_y = 0;

            if (cur_tool == TOOL_ERASER) {
                out_x = (int32_t)lrintf(eraser_x_raw);
                out_y = (int32_t)lrintf(eraser_y_raw);
                if (out_x < 0) out_x = 0;
                if (out_x > RAPT_LOGICAL_MAX_XY) out_x = RAPT_LOGICAL_MAX_XY;
                if (out_y < 0) out_y = 0;
                if (out_y > RAPT_LOGICAL_MAX_XY) out_y = RAPT_LOGICAL_MAX_XY;
                scaled_p = compute_eraser_pressure(eraser_span_mm, cfg.eraser_min_mm);

                latch.kind         = TOOL_ERASER;
                latch.x_mm         = eraser_cx_mm;
                latch.y_mm         = eraser_cy_mm;
                latch.last_seen_ms = ts;
            } else {
                const struct contact_raw *pc = &contacts[primary_stylus_slot];
                out_x = pc->x;
                out_y = pc->y;
                /* RAPT encodes optical contact width / chisel wedge roll into Tip Pressure (0..29999) */
                scaled_p = (int32_t)((uint32_t)pc->pressure * 4095U / RAPT_LOGICAL_MAX_P);
                if (scaled_p < 256) scaled_p = 256;
                compute_stylus_tilt(pc, &cfg, &tilt_x, &tilt_y);

                latch.kind         = TOOL_STYLUS;
                latch.x_mm         = pc->x_mm;
                latch.y_mm         = pc->y_mm;
                latch.last_seen_ms = ts;
            }

            if (prev_pen_tool != TOOL_NONE && prev_pen_tool != cur_tool) {
                emit_event(pen_ufd, EV_KEY, BTN_TOUCH, 0);
                emit_event(pen_ufd, EV_KEY, prev_pen_tool == TOOL_ERASER ? BTN_TOOL_RUBBER : BTN_TOOL_PEN, 0);
                emit_event(pen_ufd, EV_SYN, SYN_REPORT, 0);
            }

            emit_event(pen_ufd, EV_ABS, ABS_X, out_x);
            emit_event(pen_ufd, EV_ABS, ABS_Y, out_y);
            emit_event(pen_ufd, EV_ABS, ABS_PRESSURE, scaled_p);
            emit_event(pen_ufd, EV_ABS, ABS_TILT_X, tilt_x);
            emit_event(pen_ufd, EV_ABS, ABS_TILT_Y, tilt_y);
            emit_event(pen_ufd, EV_KEY, cur_tool == TOOL_ERASER ? BTN_TOOL_RUBBER : BTN_TOOL_PEN, 1);
            emit_event(pen_ufd, EV_KEY, BTN_TOUCH, 1);
            emit_event(pen_ufd, EV_SYN, SYN_REPORT, 0);
            prev_pen_tool = cur_tool;
        } else if (prev_pen_tool != TOOL_NONE) {
            emit_event(pen_ufd, EV_ABS, ABS_PRESSURE, 0);
            emit_event(pen_ufd, EV_KEY, BTN_TOUCH, 0);
            emit_event(pen_ufd, EV_KEY, prev_pen_tool == TOOL_ERASER ? BTN_TOOL_RUBBER : BTN_TOOL_PEN, 0);
            emit_event(pen_ufd, EV_SYN, SYN_REPORT, 0);
            prev_pen_tool = TOOL_NONE;
        }

        if (cfg.dump_frames) fflush(stdout);
    }

    ioctl(finger_ufd, UI_DEV_DESTROY);
    ioctl(pen_ufd, UI_DEV_DESTROY);
    close(finger_ufd);
    close(pen_ufd);
    for (size_t i = 0; i < num_grabbed; i++) {
        ioctl(grab_fds[i], EVIOCGRAB, 0);
        close(grab_fds[i]);
    }
    close(hid_fd);
    return 0;
}
