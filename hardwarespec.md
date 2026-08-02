# Hardware Spec — ESP32-2424S012 (Round Dashboard)

Single source of truth for the physical device, used to drive the case model
(`case/round-dashboard-case.scad`). The `.scad` parameters mirror the field
names here 1:1.

Confidence markers:
- **[DS]** — from the vendor datasheet, `docs/datasheets/esp32-2424s012-board-datasheet.pdf`
  (Shenzhen Jingcai Intelligent, "1.28 inch ESP32 module ESP32-2424S012N/C").
- **[DERIVED]** — computed from [DS] numbers; sound, but confirm.
- **[MEASURE]** — best-guess from community/teardowns; **must** be confirmed with calipers.

Units: mm. Origin for positions: **centre of the round PCB** (= centre of the
display), looking at the **front (screen) face**, +X right, +Y up.
"Back" = component/battery side.

## Datasheets (local copies)
| File | What |
|------|------|
| `docs/datasheets/esp32-2424s012-board-datasheet.pdf` | Vendor board datasheet — parameters, interface map, **mechanical drawing** |
| `docs/datasheets/esp32-c3-mini-1-datasheet.pdf` | Espressif ESP32-C3 datasheet (MCU reference) |

---

## ⚠️ Correction vs. the previous revision

The datasheet drawing (p.6) shows the board is **round with a small tab at the
bottom for the USB-C port** — *not* a rounded rectangle. The old
`board_w × board_h = 38.5 × 37` with `board_corner_r = 4` is wrong twice over:
the axes were transposed **and** the outline is a circle. See
[Case implications](#case-implications).

---

## Board
| Field | Value | Src | Notes |
|-------|-------|-----|-------|
| Model | ESP32-2424S012**N**-I | [DS] | `N` = no touch; `C` = capacitive (CST816D). SKUs: `…012N-I`, `…012C-I`, `…012C-I-Y(W/B)` (with shell) |
| MCU module | ESP32-C3-MINI-1U | [DS] | single-core RISC-V @160 MHz, 400 KB SRAM, 384 KB ROM, 4 MB flash, **no PSRAM** |
| Outline shape | **circle + bottom tab** | [DS] | tab carries the USB-C receptacle |
| Envelope `board_w × board_h` | **37.00 × 38.50** | [DS] | "Module size 38.5*37.0"; drawing: 37.00 across, 38.50 tall |
| Round-section Ø `board_dia` | **37.0** (36.2 by the tab dims) | [DS]/[DERIVED] | the two datasheet dims disagree by ~0.8 — design to 37.0 + tolerance |
| Tab width `tab_w` | **14.23** | [DS] | centred on the bottom |
| Tab height `tab_h` | **3.74** | [DS] | measured from where the tab sides meet the circle, not from the circle's lowest point |
| Enclosing radius | **≈ 21.6** (Ø ≈ 43.2) | [DERIVED] | smallest circle centred on the display that contains circle **+ tab** — this, not 18.5, sizes the skirt |
| PCB thickness `board_th` | ~1.6 | [MEASURE] | |
| Mounting holes | **none** | [DS] | drawing/interface views show no holes — the case must retain by clamping |
| Back components height `back_components` | ~2.4 (module) / ~3.2 (USB-C shell) | [MEASURE] | ESP32-C3-MINI-1U is 2.4 thick; USB-C shell is the tallest, but it sits on the tab |
| Weight | ~20 g | [DS] | |
| Operating / storage temp | −20…70 °C / −30…80 °C | [DS] | |
| Supply / draw | 5 V, ~100 mA | [DS] | |

### Outline geometry (for the `.scad`)
```
      circle Ø 37.0, centred on the display / origin
            ┌───────────────┐
        ╱                       ╲
       │            ●(0,0)       │      +Y up
        ╲                       ╱
            └──┬─────────┬──┘   ← circle bottom, y ≈ −18.5
               │ 14.23   │       tab, 3.74 tall from the chord
               └─────────┘      ← y ≈ −20.0  (total height 38.50)
```

## Display
| Field | Value | Src | Notes |
|-------|-------|-----|-------|
| Panel | 1.28" round IPS TFT, GC9A01, 240×240, 16-bit colour | [DS] | COG — driver bonded to the glass |
| **Effective display area Ø** | **32.4** | [DS] | use this for the bezel window |
| Cover-glass Ø `glass_dia` | ~35 | [MEASURE] | must be ≤ 37 (the PCB circle); measure before cutting the window |
| Glass height above PCB `glass_stand` | ~1.5 | [MEASURE] | front face of glass vs. PCB front face |
| Display ↔ PCB link | 16-pin FPC | teardown | keep the case from pinching it |
| Touch (C variant only) | CST816D on FPC (COF), I²C | [DS] | not fitted on our `N` board |

## GPIO pinout (from firmware)
| Signal | Pin | Signal | Pin |
|--------|-----|--------|-----|
| SPI CLK | GPIO6 | Display CS | GPIO10 |
| SPI MOSI | GPIO7 | Display DC | GPIO2 |
| Backlight (LEDC, via AO3402 MOSFET) | GPIO3 | Display RST | GPIO1 |
| Touch I²C SDA | GPIO4 | Touch INT | GPIO0 |
| Touch I²C SCL | GPIO5 | USB D−/D+ | GPIO18 / GPIO19 |
| **Free:** UART0 TX/RX | GPIO21 / GPIO20 | Free pad `S1`/`IO8` | GPIO8 (backup boot pad) |
| **BOOT button** | **GPIO9** (10 kΩ pull-up) | | |

## Component positions

The datasheet's interface view (p.5) is a **BACK / component-side** photo, so its
left/right are **mirrored** vs. the screen face — X-mirror it. The case body + boot
post sit on that same back side, so a feature that looks upper-**right in the photo**
is upper-**left on the screen face** (and has **negative x** in the model). p.5
carries **no dimensions** and p.6 dimensions **only the outline**, so these
coordinates are **scaled off the p.5 render (±2 mm)**: good for quadrants, *not* for
the BOOT post — jig-measure that.

| Part | p.5 photo (back view) | **Front-face coords** (X-mirrored) | Quadrant (front) |
|------|-----------------------|------------------------------------|------------------|
| ESP32-C3-MINI-1U | centred, upper half | (0, +9) | top centre |
| **BOOT** (GPIO9) | **upper-right** | **(−10, +10)** *[MEASURE]* | **upper-left** |
| RESET / EN | upper-left | (+10, +10) *[MEASURE]* | upper-right |
| Power SWITCH (IP5306 KEY) | right edge | (−16, −3) *[MEASURE]* | left edge, just below centre |
| Battery JST 1.25 2-pin | lower-right | (−11, −9) *[MEASURE]* | lower-left |
| UART JST-SH 1.0 4-pin | lower-left | (+12, −8) *[MEASURE]* | lower-right |
| **USB-C** | bottom, ~2 mm left | **(+2, −20)**, bottom edge | bottom, off-centre right |

> **BOOT is upper-RIGHT in the p.5 photo (a BACK view) → upper-LEFT on the screen
> face.** The post sits on the back side with the body, so from the back it must
> land where the photo shows BOOT (upper-right) — which is `boot_x = −10, boot_y =
> +10` in the model (screen = +z, +x = screen-right). ±2 mm from the render, so the
> post carries a **~5 mm contact pad** (`boot_pad_d`) that still strikes the ~1.5 mm
> plunger if off by up to ±2 mm. This is the one value the press-the-face click
> depends on — **jig-measure it** and set exact `boot_x/boot_y` (and `boot_sw_h`,
> the cap's protrusion below the PCB back) before committing to a print.

## Connectors (3 ports)
| Port | Type | Pinout | Position |
|------|------|--------|----------|
| USB-C | native USB (D+ G19 / D− G18) | power + data + flash | **bottom tab, ~2 mm off-centre (right on screen face)** [DS] — **racetrack** shape; case uses `usb_x = +2` + a rounded slot (`usb_angle` retired) |
| Battery | JST **1.25 mm** 2-pin | `BAT+` / `GND` | lower-**left** (screen face) *[MEASURE]* — unused (no battery), leave header empty |
| UART0 | JST-SH **1.0 mm** 4-pin | `3V3` / `GND` / `GPIO21 TX` / `GPIO20 RX` | lower-**right** (screen face) *[MEASURE]* |

## Buttons (3)
| Button | Wiring | Position (front view) |
|--------|--------|----------|
| **BOOT** | **GPIO9**, 10 kΩ pull-up (readable; published to HA) | **upper-left** (screen face; upper-right in the back-view photo) *[MEASURE]* ← drives the press-the-face click |
| RESET / EN | ESP enable, 10 kΩ pull-up (not readable) | upper-right (screen face) *[MEASURE]* |
| Power / SWITCH | IP5306 `KEY` (not wired to the ESP) | left edge (screen face) *[MEASURE]* |
| `S1` pad | GPIO8 — solder pad, backup boot | teardown |

## Power
- **USB-C only** — the case houses **no battery** (removed from the design; the puck
  has no battery pocket). The board is powered/flashed over USB-C.
- On-board still present but unused by the case: charge/boost IC **IP5306**, LDO
  **A6165P** (5 V → 3.3 V), backlight **AO3402** N-MOSFET, and the **JST 1.25 mm**
  battery header — leave the header unpopulated.
- ~~Cell 802525 (25 × 25 × 8.6 mm, 400 mAh)~~ — **not used.**

## Overall
| Field | Value | Src |
|-------|-------|-----|
| Board envelope | 37.00 × 38.50 | [DS] |
| Total assembled thickness (front glass → back) | *[MEASURE]* | — |
| Mass (board + display) | ~20 g | [DS] |

---

## Case implications

`.scad` parameters that need to change now that the outline is known:

| Param | Current | Should be | Why |
|-------|---------|-----------|-----|
| `board_w` | 38.5 | **37.0** | axes were transposed |
| `board_h` | 37.0 | **38.5** | ditto |
| `board_corner_r` | 4.0 | **18.5** | the board is round, not a rounded rectangle |
| `boot_x`, `boot_y` | −10, +10 ✓ | keep **−10, +10** | photo (back view) upper-right → screen-face upper-LEFT; ±2 mm — pad absorbs it, *jig-measure to finalise* |
| `usb_angle` → `usb_x` | 270 (put it on the LEFT!) | **retired** → bottom racetrack at `usb_x ≈ +2` | old value cut the wall at −X, not −Y; USB-C is bottom, ~2 mm off-centre (screen face right), rounded |

Two further consequences:

1. **Both the body cavity AND the bezel skirt must clear the USB tab.** The round
   part fits the round pocket (`body_ir` = 20.5), but the tab corners sit at
   **≈ 21.2** from the origin — *beyond* the round wall. So the body's interior is
   `cavity_2d()` = **round pocket ∪ a tab bay** (a `tab_w + 2·tol_loose` wide
   rounded slot reaching to the tab tip), open at the top; the board drops in from
   the front and the tab keys into the bay (also stops rotation). The bezel skirt
   already carries a matching tab notch. This keeps the Braun round puck (~Ø45)
   with only a wide port slot at the bottom, rather than growing the whole puck.
2. **No mounting holes exist**, so retention is entirely the bezel lip + skirt
   clamp. `bezel_lip = 1.0` against a ~35 glass leaves a 32.5 window, which is
   right on top of the 32.4 active area — go to `bezel_lip = 1.5` or confirm the
   glass Ø first, or the bezel will clip visible pixels.
3. **The jig module still builds a rounded square** (`rsq(board_w + 4, …)`). With
   `board_corner_r = 18.5` it becomes a circle and will no longer key onto the
   tab — add the tab to the jig outline so it can only drop on in one
   orientation, otherwise the BOOT X/Y it reports can be off by a rotation.

## Measurement checklist (calipers)
- [x] ~~PCB outline~~ — **[DS]**: Ø37 circle + 14.23 × 3.74 bottom tab, 38.5 overall
- [x] ~~Active display Ø~~ — **[DS]**: 32.4
- [ ] Confirm round-section Ø: datasheet dims imply 36.2 *or* 37.0
- [ ] PCB thickness `board_th`
- [ ] Cover-glass Ø `glass_dia` and glass height above PCB `glass_stand`
- [ ] Tallest back-component height `back_components` (module vs. USB-C shell)
- [ ] **BOOT (GPIO9) switch X/Y from PCB centre** ← use the printable jig; the click depends on it
- [ ] **BOOT cap protrusion below the PCB back** (`boot_sw_h`) — sets the post height
- [ ] RESET / power-switch positions (clearance + holes)
- [ ] USB-C slot W×H and its height above the PCB back face
- [ ] UART / battery-header connector positions (so nothing fouls the shell) — no battery fitted

## Sources
- [Vendor datasheet (PDF)](https://cdn.espboards.net/boards/cyd-esp32-2424s012/docs/datasheet.pdf) — via [espboards.dev: CYD ESP32-2424S012](https://www.espboards.dev/esp32/cyd-esp32-2424s012/)
- [Circuit Digest — ESP32-2424S012C teardown: PCB, pinout & schematic](https://circuitdigest.com/review/inside-the-esp32-2424s012c-development-board-teardown)
- [Espressif ESP32-C3-MINI-1 / -1U datasheet](https://documentation.espressif.com/esp32-c3-mini-1_datasheet_en.pdf)
- [Homeding — JCZN ESP32-2424S012 board notes](https://homeding.github.io/boards/esp32c3/jczn-esp32-2424s012.htm)
- [openHASP — ESP32-2424S012](https://www.openhasp.com/0.7.0/hardware/sunton/esp32-2424s012/)
