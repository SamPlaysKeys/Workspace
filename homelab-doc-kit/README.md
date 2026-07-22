# homelab-doc-kit

A **portable, dependency-free kit** for documenting a homelab with an AI assistant, using the four-phase interactive method (Goal → Achievable → How to → Fit).

You can drop this kit into *any* repository (or run it against an empty folder) and scaffold a fresh, self-contained homelab documentation set in seconds. No external dependencies, no links back to anyone's private lab — the output is yours.

## What it produces

Running `bootstrap.sh` copies `templates/` into your chosen directory and fills in your lab name, producing:

```
<dir>/
  README.md                # the method + loop model, as your lab's index
  phases/                  # 01 Goal, 02 Achievable, 03 How to, 04 Fit
  prompts/                 # AI-assist session scripts, one per phase
  overview/
    lab-map.template.md    # starter environment/service map
```

Every file links to the others with **relative paths**, so the set is fully self-referential once scaffolded. The only external reference is an *optional* link to a public worked example.

## Requirements

- A POSIX shell (`sh`) — that's it. `cp`, `find`, `sed` ship with every base UNIX/Linux/macOS.
- No Python, no Node, no network, no Git required (though you'll probably want to commit the result).

## Usage

```sh
# from inside the kit directory
./bootstrap.sh --name "Smith Homelab"

# custom target directory
./bootstrap.sh --name "Smith Homelab" --dir docs/infra

# overwrite an existing scaffold
./bootstrap.sh --name "Smith Homelab" --force
```

| Flag | Meaning | Default |
|------|---------|---------|
| `-n, --name` | Human-readable lab name (**required**) | — |
| `-d, --dir`  | Target directory for the scaffold | `docs/homelab` |
| `-f, --force`| Overwrite target if it exists | off |
| `-h, --help`| Show help | — |

## After scaffolding

1. Open `<dir>/README.md` — your lab's methodology index.
2. Hand `<dir>/prompts/goal.session.md` to your AI assistant. It interviews you and drafts your **Ideal State**.
3. Iterate: Achievable → How to → Fit. Each `prompts/*.session.md` tells the assistant exactly what to do and what *not* to do.
4. Fill `overview/lab-map.template.md` to visualize the lab.

## How "exportable" is this, really?

- **No hard dependencies.** The kit references only itself and one optional public URL.
- **Agent-agnostic sessions.** The `prompts/*.session.md` files are plain instructions any assistant (or a human) can run — no code, no lock-in.
- **Copy-out by design.** The whole `homelab-doc-kit/` folder is meant to be copied, forked, or vendored into another repo. That's the point.

## Worked example (optional)

This kit is self-contained — no external example is required to use it. If you publish your own populated lab docs as a worked example, drop the link here so others can see the method applied:

<!-- Optional: link a public worked example of this method here, e.g.
     For a fully worked example, see <https://your-domain.example/docs/homelab.html>. -->
