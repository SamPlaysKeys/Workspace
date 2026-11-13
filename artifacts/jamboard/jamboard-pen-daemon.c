/*
 * jamboard-pen-daemon.c — Google Jamboard (baracus) RAPT Optical Stylus/Touch Daemon
 *
 * Clean-room Linux/Android user-space driver for the Google Jamboard's RAPT Touch
 * ("HazelRed" / hazeldos, VID:PID 2986:0137 or 1cbe:0003) optical InGlass bezel.
 *
 * Why this daemon is needed:
 *   1. The RAPT controller exposes two HID interfaces on the same USB device,
 *      both streaming Input Report 0xF1 (24 contact slots):
 *        - Basic    (rdesc 2313 B): 340-byte report,  14-byte contacts.
 *        - Extended (rdesc 3149 B): 484-byte report,  20-byte contacts, where
 *          byte 0 = bit0 Tip Switch | bits1..7 = Vendor 0xFF00:0x02 CONTACT CLASS
 *          (1 = stylus, 2 = finger, 3 = eraser / large object). The extended
 *          interface also streams Report 0xF6 (103 B contamination map), which
 *          this daemon ignores.
 *   2. Unmodified Linux hid-multitouch ignores the vendor class, hardcodes every
 *      contact to MT_TOOL_FINGER, and exposes both interfaces as evdev nodes.
 *   3. This daemon grabs the raw kernel evdev nodes (EVIOCGRAB), parses Report
 *      0xF1 directly from /dev/hidraw*, and exposes two clean uinput devices:
 *        - "Jamboard RAPT Finger Touch" (24-slot Type-B Direct Multi-Touch)
 *        - "Jamboard RAPT Stylus & Eraser" (Direct Pen/Rubber Tablet with
 *          BTN_TOOL_PEN / BTN_TOOL_RUBBER, ABS_PRESSURE, open-ended universal
 *          eraser, and spatial palm/whiteboard-block contact clustering)
 *
 * Classification (measured on live LineageOS 22.2 firmware, see
 * docs/guides/jamboard/live-input-capture-findings.md):
 *   - Extended interface: use the firmware class from byte 0 bits 1..7.
 *   - Basic interface (or --sum-classifier): Width + Height is constant per class
 *     (stylus 12000, finger 26998, eraser 29998), so thresholds on Width + Height
 *     reproduce the firmware class exactly.
 *   - Width/Height are NOT physical millimetres; Height is an opaque size measure.
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
#define RAPT_MAX_SLOTS        24
#define RAPT_RDESC_BASIC_LEN  2313
#define RAPT_RDESC_EXT_LEN    3149

/* Physical active area from the live report descriptors: 1207.6 mm x 678.4 mm */
#define RAPT_LOGICAL_MAX_XY   29999
#define RAPT_LOGICAL_MAX_WH   14999
#define RAPT_LOGICAL_MAX_P    29999
#define RAPT_PHYS_WIDTH_MM    1207.6f
#define RAPT_PHYS_HEIGHT_MM   678.4f
#define RAPT_RES_X_UNITS_MM   25  /* ~29999 / 1207.6 mm */
#define RAPT_RES_Y_UNITS_MM   44  /* ~29999 / 678.4 mm  */

#define MAX_GRABBED_EVDEV     8

enum tool_kind {
    TOOL_NONE   = 0,
    TOOL_STYLUS = 1,
    TOOL_FINGER = 2,
    TOOL_ERASER = 3,
};

struct frame_layout {
    const char *name;
    int report_len;
    int contact_len;
    int count_off;
    int scan_off;
    bool has_fw_class;
};

static const struct frame_layout LAYOUT_BASIC = {"basic",    340, 14, 337, 338, false};
static const struct frame_layout LAYOUT_EXT   = {"extended", 484, 20, 481, 482, true};

struct daemon_cfg {
    const char *hidraw_path;
    bool grab_evdev;
    bool dump_frames;
    bool verbose;
    bool sum_classifier;
    int sum_stylus_max;
    int sum_finger_max;
    float eraser_base_mm;
    float eraser_cluster_mm;
    float latch_radius_mm;
    int latch_ms;
};

struct contact_raw {
    bool active;
    uint8_t cid;
    uint8_t fw_class;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint16_t pressure;
    float x_mm;
    float y_mm;
    enum tool_kind cls;
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

static const struct frame_layout *layout_for_rdesc(int rdesc_size) {
    if (rdesc_size == RAPT_RDESC_EXT_LEN) return &LAYOUT_EXT;
    if (rdesc_size == RAPT_RDESC_BASIC_LEN) return &LAYOUT_BASIC;
    return NULL;
}

/* Prefer the extended (3149-byte) interface because it carries the firmware class. */
static const struct frame_layout *find_rapt_hidraw(char *out_path, size_t out_len) {
    DIR *dir = opendir("/dev");
    if (!dir) return NULL;

    char basic_path[128] = {0};
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strncmp(de->d_name, "hidraw", 6) != 0) continue;

        char cand[128];
        snprintf(cand, sizeof(cand), "/dev/%s", de->d_name);
        int fd = open(cand, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        struct hidraw_devinfo info;
        int rdesc_size = 0;
        if (ioctl(fd, HIDIOCGRAWINFO, &info) == 0 &&
            is_rapt_vid_pid(info.vendor & 0xFFFF, info.product & 0xFFFF) &&
            ioctl(fd, HIDIOCGRDESCSIZE, &rdesc_size) == 0) {
            if (rdesc_size == RAPT_RDESC_EXT_LEN) {
                snprintf(out_path, out_len, "%s", cand);
                close(fd);
                closedir(dir);
                return &LAYOUT_EXT;
            }
            if (rdesc_size == RAPT_RDESC_BASIC_LEN && basic_path[0] == '\0') {
                snprintf(basic_path, sizeof(basic_path), "%s", cand);
            }
        }
        close(fd);
    }
    closedir(dir);

    if (basic_path[0] != '\0') {
        snprintf(out_path, out_len, "%s", basic_path);
        return &LAYOUT_BASIC;
    }
    return NULL;
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
    setup_abs(ufd, ABS_MT_TOUCH_MAJOR, 0, RAPT_LOGICAL_MAX_WH, 0);
    setup_abs(ufd, ABS_MT_TOUCH_MINOR, 0, RAPT_LOGICAL_MAX_WH, 0);
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

static enum tool_kind class_from_sum(const struct contact_raw *c, const struct daemon_cfg *cfg) {
    int sum = (int)c->w + (int)c->h;
    if (sum < cfg->sum_stylus_max) return TOOL_STYLUS;
    if (sum < cfg->sum_finger_max) return TOOL_FINGER;
    return TOOL_ERASER;
}

static enum tool_kind classify_contact(const struct contact_raw *c,
                                       const struct frame_layout *layout,
                                       const struct daemon_cfg *cfg) {
    if (layout->has_fw_class && !cfg->sum_classifier &&
        c->fw_class >= TOOL_STYLUS && c->fw_class <= TOOL_ERASER) {
        return (enum tool_kind)c->fw_class;
    }
    return class_from_sum(c, cfg);
}

/* New contacts landing next to a just-lifted eraser stay erasers (mirrors the stock
 * mEraserLatchRange / mEraserDelayTime behaviour in libinputflinger). */
static enum tool_kind apply_eraser_latch(enum tool_kind cls,
                                         const struct contact_raw *c,
                                         const struct daemon_cfg *cfg,
                                         const struct tool_latch *latch,
                                         int64_t ts_ms) {
    if (cls != TOOL_ERASER && latch->kind == TOOL_ERASER &&
        (ts_ms - latch->last_seen_ms) <= cfg->latch_ms &&
        hypotf(c->x_mm - latch->x_mm, c->y_mm - latch->y_mm) <= cfg->latch_radius_mm) {
        return TOOL_ERASER;
    }
    return cls;
}

static int32_t compute_eraser_pressure(float span_mm, float base_mm) {
    /* Map eraser cluster span [base_mm .. 180mm] -> [1200 .. 4095]
     * so pressure-sensitive whiteboard apps widen the eraser swath for palms & blocks */
    float denom = 180.0f - base_mm;
    if (denom < 10.0f) denom = 10.0f;
    float t = (span_mm - base_mm) / denom;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return 1200 + (int32_t)lrintf(t * 2895.0f);
}

static uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static const char *kind_name(enum tool_kind k) {
    switch (k) {
        case TOOL_STYLUS: return "STYLUS";
        case TOOL_FINGER: return "FINGER";
        case TOOL_ERASER: return "ERASER";
        default:          return "NONE";
    }
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --hidraw <path>            Explicit /dev/hidrawX path (auto-detected by default;\n"
        "                             the 3149-byte extended interface is preferred)\n"
        "  --no-grab                  Do not EVIOCGRAB raw RAPT kernel evdev nodes\n"
        "  --dump                     Print decoded active contacts to stdout\n"
        "  --verbose                  Verbose startup and tool transition logging\n"
        "  --sum-classifier           Ignore the firmware class; classify by Width+Height\n"
        "  --sum-stylus-max <int>     Width+Height below this is a stylus (default: 18000)\n"
        "  --sum-finger-max <int>     Width+Height below this is a finger, else eraser (default: 27000)\n"
        "  --eraser-base-mm <float>   Nominal eraser size used for swath pressure (default: 40.0)\n"
        "  --eraser-cluster-mm <flt>  Capture radius mm to merge palm/block blobs (default: 70.0)\n"
        "  -h, --help                 Show this help\n",
        prog);
}

int main(int argc, char **argv) {
    struct daemon_cfg cfg = {
        .hidraw_path       = NULL,
        .grab_evdev        = true,
        .dump_frames       = false,
        .verbose           = false,
        .sum_classifier    = false,
        .sum_stylus_max    = 18000,
        .sum_finger_max    = 27000,
        .eraser_base_mm    = 40.0f,
        .eraser_cluster_mm = 70.0f,
        .latch_radius_mm   = 55.0f,
        .latch_ms          = 150,
    };

    static const struct option long_opts[] = {
        {"hidraw",             required_argument, NULL, 'd'},
        {"no-grab",            no_argument,       NULL, 'G'},
        {"dump",               no_argument,       NULL, 'D'},
        {"verbose",            no_argument,       NULL, 'v'},
        {"sum-classifier",     no_argument,       NULL, 'S'},
        {"sum-stylus-max",     required_argument, NULL, 's'},
        {"sum-finger-max",     required_argument, NULL, 'f'},
        {"eraser-base-mm",     required_argument, NULL, 'B'},
        {"eraser-cluster-mm",  required_argument, NULL, 'C'},
        {"help",               no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:GDvSs:f:B:C:h", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'd': cfg.hidraw_path = optarg; break;
            case 'G': cfg.grab_evdev = false; break;
            case 'D': cfg.dump_frames = true; break;
            case 'v': cfg.verbose = true; break;
            case 'S': cfg.sum_classifier = true; break;
            case 's': cfg.sum_stylus_max = atoi(optarg); break;
            case 'f': cfg.sum_finger_max = atoi(optarg); break;
            case 'B': cfg.eraser_base_mm = strtof(optarg, NULL); break;
            case 'C': cfg.eraser_cluster_mm = strtof(optarg, NULL); break;
            case 'h':
            default:
                usage(argv[0]);
                return opt == 'h' ? 0 : 1;
        }
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    const struct frame_layout *layout = NULL;
    char auto_hidraw[128] = {0};
    if (!cfg.hidraw_path) {
        layout = find_rapt_hidraw(auto_hidraw, sizeof(auto_hidraw));
        if (!layout) {
            fprintf(stderr, "[jamboard-pen] ERROR: RAPT hidraw device not found (need root?).\n");
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

    if (!layout) {
        int rdesc_size = 0;
        ioctl(hid_fd, HIDIOCGRDESCSIZE, &rdesc_size);
        layout = layout_for_rdesc(rdesc_size);
        if (!layout) {
            fprintf(stderr,
                    "[jamboard-pen] WARNING: unexpected report descriptor size %d; assuming basic layout.\n",
                    rdesc_size);
            layout = &LAYOUT_BASIC;
        }
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
            "[jamboard-pen] Active on %s (%s layout, %s classifier, grabbed %zu raw evdev node(s)). Streaming to uinput.\n",
            cfg.hidraw_path, layout->name,
            (layout->has_fw_class && !cfg.sum_classifier) ? "firmware" : "Width+Height",
            num_grabbed);

    struct slot_tracker slots[RAPT_MAX_SLOTS];
    memset(slots, 0, sizeof(slots));
    struct tool_latch latch = {0};
    int next_tracking_id = 1;
    enum tool_kind prev_pen_tool = TOOL_NONE;

    uint8_t buf[1024];
    while (!g_stop) {
        ssize_t r = read(hid_fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("[jamboard-pen] hidraw read");
            break;
        }
        /* Skips Report 0xF6 (contamination map) and any short/foreign reports */
        if (r < layout->report_len || buf[0] != RAPT_REPORT_ID) {
            continue;
        }

        int64_t ts = now_ms();
        uint8_t contact_count = buf[layout->count_off];
        uint16_t scan_time = get_u16_le(&buf[layout->scan_off]);

        struct contact_raw contacts[RAPT_MAX_SLOTS];
        int seed_eraser_slot = -1;
        uint16_t seed_eraser_size = 0;

        /* Pass 1: Decode all 24 slots, retire lifted contacts, and classify tools */
        for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
            const uint8_t *s = &buf[1 + i * layout->contact_len];
            struct contact_raw *c = &contacts[i];
            c->active    = (s[0] & 0x01) != 0;
            c->fw_class  = layout->has_fw_class ? (s[0] >> 1) : 0;
            c->cid       = s[1];
            c->x         = get_u16_le(&s[2]);
            c->y         = get_u16_le(&s[4]);
            c->w         = get_u16_le(&s[6]);
            c->h         = get_u16_le(&s[8]);
            c->pressure  = get_u16_le(&s[10]);

            c->x_mm      = (float)c->x * RAPT_PHYS_WIDTH_MM / 30000.0f;
            c->y_mm      = (float)c->y * RAPT_PHYS_HEIGHT_MM / 30000.0f;
            c->cls       = classify_contact(c, layout, &cfg);

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
                slots[i].kind               = apply_eraser_latch(c->cls, c, &cfg, &latch, ts);
                if (cfg.verbose && slots[i].kind != TOOL_FINGER) {
                    fprintf(stderr,
                            "[jamboard-pen] Slot %d cid=%u -> %s (W=%u H=%u P=%u fw_class=%u)\n",
                            i, c->cid, kind_name(slots[i].kind),
                            c->w, c->h, c->pressure, c->fw_class);
                }
            } else if (slots[i].kind != TOOL_ERASER && c->cls == TOOL_ERASER) {
                slots[i].kind = TOOL_ERASER;
                if (slots[i].sent_as_finger) {
                    emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                    emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                    slots[i].sent_as_finger = false;
                }
                if (cfg.verbose) {
                    fprintf(stderr,
                            "[jamboard-pen] Slot %d cid=%u promoted -> ERASER (W=%u H=%u)\n",
                            i, c->cid, c->w, c->h);
                }
            }

            if (slots[i].kind == TOOL_ERASER && (seed_eraser_slot < 0 || c->h > seed_eraser_size)) {
                seed_eraser_slot = i;
                seed_eraser_size = c->h;
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
                    float weight = contacts[i].h > 100 ? (float)contacts[i].h : 100.0f;
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

            /* Overall swath span (mirrors TouchGroup::GetRadius @ 0x00476e04): widest centroid
             * distance of any member, plus the nominal eraser size. Width/Height are not
             * physical, so blob extent is not used. */
            float max_rad_mm = 0.0f;
            for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
                if (!in_eraser_cluster[i]) continue;
                float r_mm = hypotf(contacts[i].x_mm - eraser_cx_mm,
                                    contacts[i].y_mm - eraser_cy_mm);
                if (r_mm > max_rad_mm) max_rad_mm = r_mm;
            }
            eraser_span_mm = 2.0f * max_rad_mm + cfg.eraser_base_mm;
        }

        int primary_stylus_slot = -1;
        int active_fingers = 0;
        int first_finger_slot = -1;

        for (int i = 0; i < RAPT_MAX_SLOTS; i++) {
            const struct contact_raw *c = &contacts[i];
            if (!c->active) continue;

            if (cfg.dump_frames) {
                printf("scan=%5u cnt=%2u slot=%2d cid=%3u X=%5u Y=%5u W=%5u H=%4u P=%5u fw=%u sum=%s -> %s\n",
                       scan_time, contact_count, i, c->cid, c->x, c->y,
                       c->w, c->h, c->pressure, c->fw_class,
                       kind_name(class_from_sum(c, &cfg)),
                       in_eraser_cluster[i] ? "ERASER_CLUSTER" :
                       (slots[i].absorbed_by_eraser ? "ERASER_TRAIL" :
                       kind_name(slots[i].kind)));
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

                emit_event(finger_ufd, EV_ABS, ABS_MT_SLOT, i);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TRACKING_ID, slots[i].tracking_id);
                emit_event(finger_ufd, EV_ABS, ABS_MT_POSITION_X, c->x);
                emit_event(finger_ufd, EV_ABS, ABS_MT_POSITION_Y, c->y);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TOUCH_MAJOR, c->h);
                emit_event(finger_ufd, EV_ABS, ABS_MT_TOUCH_MINOR, c->h);
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
            int32_t out_x, out_y, scaled_p;

            if (cur_tool == TOOL_ERASER) {
                out_x = (int32_t)lrintf(eraser_x_raw);
                out_y = (int32_t)lrintf(eraser_y_raw);
                if (out_x < 0) out_x = 0;
                if (out_x > RAPT_LOGICAL_MAX_XY) out_x = RAPT_LOGICAL_MAX_XY;
                if (out_y < 0) out_y = 0;
                if (out_y > RAPT_LOGICAL_MAX_XY) out_y = RAPT_LOGICAL_MAX_XY;
                scaled_p = compute_eraser_pressure(eraser_span_mm, cfg.eraser_base_mm);

                latch.kind         = TOOL_ERASER;
                latch.x_mm         = eraser_cx_mm;
                latch.y_mm         = eraser_cy_mm;
                latch.last_seen_ms = ts;
            } else {
                const struct contact_raw *pc = &contacts[primary_stylus_slot];
                out_x = pc->x;
                out_y = pc->y;
                /* RAPT Tip Pressure (0..29999) tracks optical contact area, not force */
                scaled_p = (int32_t)((uint32_t)pc->pressure * 4095U / RAPT_LOGICAL_MAX_P);
                if (scaled_p < 256) scaled_p = 256;
            }

            if (prev_pen_tool != TOOL_NONE && prev_pen_tool != cur_tool) {
                emit_event(pen_ufd, EV_KEY, BTN_TOUCH, 0);
                emit_event(pen_ufd, EV_KEY, prev_pen_tool == TOOL_ERASER ? BTN_TOOL_RUBBER : BTN_TOOL_PEN, 0);
                emit_event(pen_ufd, EV_SYN, SYN_REPORT, 0);
            }

            emit_event(pen_ufd, EV_ABS, ABS_X, out_x);
            emit_event(pen_ufd, EV_ABS, ABS_Y, out_y);
            emit_event(pen_ufd, EV_ABS, ABS_PRESSURE, scaled_p);
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
