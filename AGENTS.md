# AGENTS.md

Guidance for AI coding agents working in this repo. Human docs live in `README.md`;
this file is the working brief.

## What this is

An ESPHome firmware config that turns an **ESP32-2424S012** (round 1.28" **GC9A01**,
240×240, ESP32-C3) into a Home Assistant "Swiss-watch" dashboard, plus a parametric
**OpenSCAD** 3D-printed case. Two independent tracks share one Makefile:

1. **Firmware** — the watch-face display, HA-driven, OTA-flashed.
2. **Case** — a Braun/Apple-style white puck rendered/exported from OpenSCAD.

## Layout

| Path | What it is |
|------|-----------|
| `round-dashboard.yaml` | The ESPHome config: device, entities, HA controls, and the display `lambda`. |
| `round_dashboard.h` | C++ drawing/theme helpers in `namespace rd`, called from the lambda. |
| `Makefile` | Build/flash/logs **and** case render/STL targets. |
| `docker-compose.yml` | ESPHome container the Makefile wraps. |
| `secrets.yaml` | WiFi creds — **gitignored**, never commit. `secrets.yaml.sample` is the template. |
| `case/round-dashboard-case.scad` | Parametric enclosure (body + bezel + pcb/jig refs). |
| `case/stl/`, `case/renders/` | STL exports (committed); PNG previews (gitignored). |
| `hardwarespec.md` | Physical board spec — the single source of truth for case geometry. |
| `docs/datasheets/` | Vendor PDFs backing `hardwarespec.md`. |

## Build & run

Everything goes through `make` (wraps Docker for ESPHome, local OpenSCAD for the case).
Do **not** assume a local ESPHome/Python install.

```bash
make build                         # compile firmware only (no device) — use this to verify YAML/C++ changes
make run device=10.12.13.192       # compile + OTA-flash (device IP required; mDNS doesn't resolve from Docker)
make logs device=10.12.13.192      # stream device logs
make render                        # case preview PNGs -> case/renders/
make stl                           # print-ready STLs -> case/stl/ (needs local OpenSCAD)
```

- `file=<name>.yaml` overrides the config; `device=<ip>` targets an address.
- **You cannot flash from here without hardware.** After edits, verify with `make build`
  (compile check) — that's the furthest an agent can validate the firmware unattended.
- First-ever flash must be USB (not possible via Docker on macOS); everything after is OTA.

## Firmware conventions

- **Keep the lambda thin.** `round-dashboard.yaml`'s display `lambda` reads HA state and
  calls `rd::` helpers; drawing/geometry/theme logic belongs in `round_dashboard.h`.
- **Theming:** all colors come from `rd::make_palette(dark)` — add a `Palette` field rather
  than hardcoding a `Color(...)` in the lambda. Every color has a dark and light value.
- **HA entities** are set in the `substitutions:` block; the four gauges' min/max are
  runtime-adjustable `number:` entities read fresh each frame.
- **Draw order matters:** background → ring/bezel → gauges → text → **seconds tick last**
  (it sits on top). The alert rings (presence=red, BOOT button=green) replace the bezel.
- The display refreshes at `update_interval: 1s`; the BOOT button forces `id(disp).update()`
  on both edges so short taps still show.
- **Pins are fixed by the board** — see the pin table in `README.md`. GPIO8/9 are strapping
  pins; don't hold them low at power-on.

## Case conventions

- Geometry is **driven by `hardwarespec.md`**, itself derived from `docs/datasheets/`. If a
  dimension changes, update `hardwarespec.md` and the `.scad` params together.
- Values tagged `[MEASURE]` (esp. `boot_x`/`boot_y`, `glass_dia`) are estimates to confirm
  with calipers / the printed `jig` — don't treat them as exact.
- **Print orientation:** visible faces are kept flat and printed against the plate; edges get
  **chamfers, not fillets** (printable face-down, no supports). The bezel exports flipped
  (flat show-face down) via the scad's `bezel_print` branch — `make stl` handles this.
- Tolerances (`tol_press`/`tol_slip`/`tol_loose`/`rim_gap`) live in one place in the `.scad`;
  change them there, not inline.

## Safety

- Never commit `secrets.yaml` or the WiFi/OTA credentials.
- The `api:` encryption key and fallback AP password are in the tracked YAML; leave them unless
  asked to rotate.
- Don't push or flash to hardware unless explicitly asked — `make build` is the safe default.
