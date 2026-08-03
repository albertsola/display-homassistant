# Dashboard Solar — spec

An ESPHome **solar-energy** dashboard for the round display, in the **same watch-face
style as `round-dashboard`** (concentric ring gauges + sub-dials + a centre readout, dark/
light themed) — but showing a home solar system: **Solar, Battery, Grid, Home**, and, on
the inner ring, **where the energy is flowing** right now. Home-Assistant-driven.

Companion to `round-dashboard`; same board, toolchain, and `rd::` drawing helpers. This
file is the single source of truth for the design — build it up here, then implement
`dashboard-solar.yaml` (+ helpers) against it.

Confidence markers, like `hardwarespec.md`: **[SET]** decided · **[DECIDE]** open design
choice · **[TBD]** needs a value/entity from the user's HA.

---

## Focus — what the screen is about  [SET]

Primary content, always on screen and glanceable:

1. **Solar** — power the panels are producing now (W) + today's kWh.
2. **Battery** — **state of charge %** and charge/discharge (signed W).
3. **Home usage** — current household consumption (W).
4. **Grid** — importing vs exporting, and how much (signed W).

Plus the signature element — the **inner flow ring** — which shows *where the energy is
going*, exploiting the invariant below.

## Hardware  [SET]

Same device as `round-dashboard`: **ESP32-2424S012N** — round **1.28" GC9A01**, 240×240,
ESP32-C3, USB-C powered, no touch (`N`). Board spec in the repo's `hardwarespec.md`; the
printed enclosures in `case/` apply unchanged (`flat_case`, `angular_stand`). Sole physical
input is the **GPIO9 BOOT** press.

## Key invariant  [SET]

**100% of solar is always used** — PV is never curtailed, so at every instant it is fully
allocated across house / battery / grid. This makes the flows fully determined by the four
base readings (no ambiguous heuristic).

**The two rings, at a glance:** the **outer ring = energy SOURCES** (where the house's power
comes from) and the **inner ring = energy USAGE** (what it is spent on). Both share one
`power_max` scale.

## Screen layout (round 240×240) — round-dashboard style  [SET, details DECIDE]

Mirrors round-dashboard's skeleton (outer bezel + two concentric rings + two sub-dials +
centre readout + on-top second tick) and reuses the `rd::` helpers + dark/light `Palette`.

```
 ┌ bezel = 60 Swiss ticks; SECOND tick — ring GREEN (BOOT pressed) / RED (presence) ┐
 │  ┌ Sources ring (outer) ─────────────── Usage ring (inner) ┐              │
 │  │                     ☀  ← weather                                         ││
 │  │                 Sat, 3 Aug                                               ││
 │  │                 12:34         ← centre clock (HH:MM)                      ││
 │  │        🔋62%    third ring: BAT left / EV right    🚗78%                 ││
 │  │           0.94 kW  ← home usage (inner ring) + connector arc             ││
 │  │           1.82 kW  ← solar (outer ring) + connector arc ─────────────────┘│
 └──────────────────────────────────────────────────────────────────────────────┘
```

| Element (geometry from round-dashboard) | dashboard-solar role |
|---|---|
| **Outer ring** (r96–110) | **Energy sources** — stacked: 🟡 solar self-used (`pv − export`) + 🟠 battery→house (discharge) + 🟥 grid import + ⚪ export (last); shared `power_max` scale. Bottom **value label** `pv` at y220, **connector arc** (r103) |
| **Inner ring** (r78–92) | **Energy usage** — stacked 🔵 house (− EV) + 🟢 home-battery charge + 🟣 EV charge. Bottom **value label** = house `load` at y202, **connector arc** (r85) |
| **Third ring** (r≈52–64) | **Batteries**, split & rising to the top: 🔋 home battery (left, fills lower-left→up) + 🚗 EV (right, fills lower-right→up, i.e. right→left). MDI **icon + `%`** on each side; ~40° top gap for the weather glyph, wide bottom gap for the labels. Replaces the two old sub-dials. |
| **Centre** (y96/y118) | **Date + `HH:MM`** clock (no seconds; home usage labels the inner ring) |
| **Top glyph** (y57) | **Weather icon** (apt — solar tracks weather) |
| **Bezel** | Swiss 60-tick ring with a seconds tick; whole ring goes **green** while the **BOOT button** is pressed, **red** on **presence** (matches round-dashboard) |

## Rings — sources (outer) + usage (inner)  [SET]

Two stacked rings on one shared `power_max` scale (arc length = actual watts).

**Outer ring — energy SOURCES** (where the house's power comes from):

| Colour | Segment | Watts |
|--------|---------|-------|
| 🟡 **amber** | solar produced, self-used | `pv − export` |
| 🟠 **dark orange** | battery → house (discharge) | `discharge` |
| 🟥 **red** | grid import | `import` |
| ⚪ **light grey** | exported to the grid (last) | `export` |

**Inner ring — energy USAGE** (what the power is spent on):

| Colour | Segment | Watts |
|--------|---------|-------|
| 🔵 **blue** | house consumption, excluding the EV | `load − ev_charge` |
| 🟢 **green** | home battery charge | `charge` |
| 🟣 **purple** | EV charge | `ev_charge` |

Every value is a **measured** sensor (charge, discharge, import, export, `ev_charge`), so
nothing is derived. `power_max` is a shared full-scale (≥ expected peak) so neither ring
overflows — a config `number:` like round-dashboard's gauge maxes.

```
// outer (sources)
self_used = max(0, pv - export);   discharge;   import;   export
// inner (usage)
house_ev  = max(0, load - ev_charge);   charge;   ev_charge
// each arc angle = watts / power_max * full_sweep
```

## Data — Home Assistant entities  [SET]

Pulled over the ESPHome API like `round-dashboard` (set in `substitutions:`).

| Field | Entity | Notes |
|-------|--------|-------|
| Solar production `pv` | `sensor.powermon_totalsolar` | live W; outer 🟡 (`pv−export`) + inner |
| House usage `load` | `sensor.consum_total` | live W (incl. EV); inner 🔵 (`load−ev`) |
| Home battery SoC (%) | `sensor.byd_battery_box_premium_hv_state_of_charge` | third ring, left |
| Battery charge (W) | `sensor.solarnet_power_battery_charge` | inner 🟢 |
| Battery discharge (W) | `sensor.solarnet_power_battery_discharge` | outer 🟠 (battery→house) |
| Grid **export** (W) | `sensor.solarnet_power_grid_export` | outer ⚪ light grey |
| Grid **import** (W) | `sensor.solarnet_power_grid_import` | outer 🟥 red |
| EV charge (W) | `sensor.kitt_charger_power` | inner 🟣 (EV charge) |
| EV battery SoC (%) | `sensor.kitt_battery` | third ring, right |
| Presence | `binary_sensor.cam_entrada_moviment_3` | red outer ring |
| Weather (icon + temp + humidity) | `weather.ivallm3` | top: condition icon, `temperature`/`humidity` attrs |

Notes:
- **All flows are measured** — grid import/export and battery charge/discharge are each their
  own non-negative sensor, so no sign conventions and no derivation. Net battery power for the
  sub-dial = `charge − discharge`; `blue` (solar→house) = `pv − export − charge`.
- **Two batteries on the sub-dials:** home battery SoC (left), EV car SoC (right). Grid
  import/export are shown on the rings, so neither needs a sub-dial.
- Optional/missing: **PV today (kWh)** for an outer-ring subtitle — not wired [TBD].

Data source: **Home Assistant**; Fronius/SolarNet inverter + BYD home battery + EV.

## Controls exposed to HA  [IMPLEMENTED]

- **Dark Mode**, **Screen** on/off, **Brightness**, **Screen Rotation** — reused from round-dashboard.
- **Power full-scale** (`power_max`, number) — the shared ring scale (batteries are 0–100%).
- **Boot Button** (binary sensor) — published to HA; a press also flashes the green ring.

Gauges always use the **segmented tick** style (rendered with filled polygons, no gaps). The
solid-fill path still exists in `dashboard_solar.h` (the `ticked` arg) but isn't exposed — the
lambda passes `true` everywhere, so re-enabling it later is a one-line change.

## Firmware structure  [IMPLEMENTED]

This folder is **independent and self-contained** (`dashboard-solar.yaml`, `dashboard_solar.h`,
`Makefile`, `docker-compose.yml`, `secrets.yaml.sample`). Build from inside the folder:

```bash
cd dashboard-solar
cp secrets.yaml.sample secrets.yaml   # fill in Wi-Fi
make build                            # or: make run device=<ip>
```

- **`dashboard-solar.yaml`** — ESPHome config (`includes: dashboard_solar.h`; `!secret` from
  this folder's `secrets.yaml`).
- **`dashboard_solar.h`** — self-contained `ds::` helper header (own `Palette` +
  `make_palette`, and copies of the primitives it needs: `draw_bezel`, `draw_alert_ring`,
  `draw_seconds_tick`, `draw_ring_gauge`, `draw_stacked_ring`, `draw_split_gauge`,
  `draw_sub_gauge`, `arc_line`, `draw_connector`, `scale_value`, `weather_glyph`, `fmt_power`,
  `nz`). Kept separate from round-dashboard so the two tracks evolve independently.
- **Ring primitives:** `draw_stacked_ring` (coloured segments, generation/consumption rings)
  and `draw_split_gauge` (the third ring, split BAT-left / EV-right rising to the top) — both
  built on `fill_arc` polygon fills.
- Lambda reads the measured sensors, builds the segments, and draws: bezel (or green
  BOOT-press / red presence ring) → outer generation ring → inner consumption ring → the two
  ring value labels + connector arcs → weather icon → centre `HH:MM` clock → split battery/
  EV third ring (icon + % each side) → seconds tick. Thin lambda; geometry/theme in the header.

**Status:** `make build` compiles clean (Flash ~60%). Not yet flashed to hardware; entity
values and on-screen placement unverified against live HA.

## Open questions

1. Exact dark/light **hex** for the colours (amber + blue/green/purple/orange/dark-red + the green/red rings).
2. Grid↔battery flows (grid charging / battery export) — keep **off**?
3. `power_max` full-scale value (≥ peak PV and peak load).

*Resolved:* PV/load are live W; grid & battery are all measured non-negative sensors (no
signs, no derivation); centre = clock; bezel = Swiss ticks, green on BOOT press, red on
presence; entity IDs known (BYD + Fronius SolarNet + EV `kitt_battery`).
