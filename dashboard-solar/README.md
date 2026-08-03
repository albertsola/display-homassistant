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

**Corollary that anchors the UI:** the **generation ring** (outer) shows total PV split into
🟡 self-used (`pv − export`) + 🟣 exported (`export`). The **consumption ring** (inner) shows
where energy is used, **excluding export**; its solar part (🔵 solar→house + 🟢 solar→battery)
equals the outer ring's yellow (self-used) segment. Export appears **only** on the generation
ring. Both rings share one `power_max` scale, so the used-solar portions line up.

## Screen layout (round 240×240) — round-dashboard style  [SET, details DECIDE]

Mirrors round-dashboard's skeleton (outer bezel + two concentric rings + two sub-dials +
centre readout + on-top second tick) and reuses the `rd::` helpers + dark/light `Palette`.

```
 ┌ bezel = 60 Swiss ticks; SECOND tick — ring GREEN (BOOT pressed) / RED (presence) ┐
 │  ┌ Generation ring (outer: 🟡 self-used + 🟣 export) ── Consumption (inner) ┐│
 │  │                     ☀  ← weather                                         ││
 │  │                 Sat, 3 Aug                                               ││
 │  │                 12:34:56      ← centre clock                             ││
 │  │              (🔋 62%)   (🚗 78%)   ← sub-dials (home batt / Tesla)        ││
 │  │           0.94 kW  ← home usage (inner ring) + connector arc             ││
 │  │           1.82 kW  ← solar (outer ring) + connector arc ─────────────────┘│
 └──────────────────────────────────────────────────────────────────────────────┘
```

| Element (geometry from round-dashboard) | dashboard-solar role |
|---|---|
| **Outer ring** (r96–110) | **Solar generation** — stacked: 🟡 sun-yellow (`pv − export`, self-used) + 🟣 purple (`export`); shared `power_max` scale. Bottom **value label** `pv` at y220 with a **connector arc** (r103) |
| **Inner ring** (r78–92) | **Consumption flow** — stacked blue/green/orange/dark-red on the same scale, **export excluded**. Bottom **value label** = home usage `load` at y202 with a **connector arc** (r85) |
| **Left sub-dial** (90,152) | **Home battery** — fill = SoC %, big `62%`, small signed `±kW` (green charge / orange discharge) |
| **Right sub-dial** (150,152) | **Tesla battery** — fill = SoC %, big `78%`, label `TESLA` (`sensor.kitt_battery`) |
| **Centre** (y96/y118) | **Date + time** clock (home usage now labels the inner ring) |
| **Top glyph** (y57) | **Weather icon** (apt — solar tracks weather) |
| **Bezel** | Swiss 60-tick ring with a seconds tick; whole ring goes **green** while the **BOOT button** is pressed, **red** on **presence** (matches round-dashboard) |

## Rings — generation (outer) + consumption (inner)  [SET]

Two stacked rings on one shared `power_max` scale (arc length = actual watts).

**Generation ring (outer)** — total solar `pv`, split by where it goes:

| Colour | Segment | Watts |
|--------|---------|-------|
| 🟡 **sun-yellow** | self-used generation (house + battery) | `pv − export` |
| 🟣 **purple** | exported to the grid | `export` |

Sum = `pv`.

**Consumption ring (inner)** — where energy is actually used/stored, **export excluded**:

| Colour | Flow | Present when |
|--------|------|--------------|
| 🔵 **blue** | solar → house | PV > 0 |
| 🟢 **green** | solar → battery (charging) | **surplus** (PV ≥ load) |
| 🟠 **orange** | battery → house | **deficit** (PV < load) |
| 🟥 **dark red** | grid → house (import) | **deficit** |

- The inner **blue + green** (self-used solar) equals the outer **yellow** segment, so the two
  rings line up on the used-solar portion; export sits only on the outer ring.
- **Surplus:** inner = blue + green ( = `pv − export`).
- **Deficit:** inner = blue + orange + dark red ( = `load`); green = 0.
- **Night (PV = 0):** inner = orange + dark red; outer ring empty.

Exact dark/light hex for the colours live as `Palette` pairs — no hardcoded `Color()`.
Precise shades **[DECIDE]**.

### Segment math (watts, plotted on the shared `power_max` scale)  [SET]

Every flow is now a **measured** sensor; nothing is derived. Export is drawn on the
generation ring; the consumption segments exclude it.

```
purple  = export;      dark_red = import;    // grid: export (outer ring) + import
green   = charge;      orange   = discharge; // battery flows (both measured)
blue    = pv - export - charge;              // solar -> house (remainder); clamp >= 0
batt    = charge - discharge;                // net for the battery sub-dial (+chg / -dis)
// draw order (inner): blue, green, orange, dark_red   (purple lives on the outer ring)
// each arc angle = watts / power_max * full_sweep
```
`power_max` is a shared full-scale (≥ expected peak PV **and** peak load) so neither ring
overflows — a config `number:` like round-dashboard's gauge maxes.

## Data — Home Assistant entities  [SET]

Pulled over the ESPHome API like `round-dashboard` (set in `substitutions:`).

| Field | Entity | Notes |
|-------|--------|-------|
| Solar production `pv` | `sensor.powermon_totalsolar` | live W [SET] |
| House usage `load` | `sensor.consum_total` | live W [SET] |
| Home battery SoC (%) | `sensor.byd_battery_box_premium_hv_state_of_charge` | left sub-dial |
| Battery charge (W) | `sensor.solarnet_power_battery_charge` | ≥ 0 → 🟢 green |
| Battery discharge (W) | `sensor.solarnet_power_battery_discharge` | ≥ 0 → 🟠 orange |
| Grid **export** (W) | `sensor.solarnet_power_grid_export` | ≥ 0 → 🟣 purple (generation ring) |
| Grid **import** (W) | `sensor.solarnet_power_grid_import` | ≥ 0 → 🟥 dark red |
| Tesla battery SoC (%) | `sensor.kitt_battery` | right sub-dial |
| Presence | `binary_sensor.cam_entrada_moviment_3` | red outer ring |
| Weather | `weather.forecast_home` [TBD] | top icon |

Notes:
- **All flows are measured** — grid import/export and battery charge/discharge are each their
  own non-negative sensor, so no sign conventions and no derivation. Net battery power for the
  sub-dial = `charge − discharge`; `blue` (solar→house) = `pv − export − charge`.
- **Two batteries on the sub-dials:** home battery SoC (left), Tesla car SoC (right). Grid
  import/export are shown on the rings, so neither needs a sub-dial.
- Optional/missing: **PV today (kWh)** for an outer-ring subtitle — not wired [TBD].

Data source: **Home Assistant**; Fronius/SolarNet inverter + BYD home battery + Tesla.

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
  `draw_seconds_tick`, `draw_ring_gauge`, `draw_sub_gauge`, `arc_line`, `draw_connector`,
  `scale_value`, `weather_glyph`, `fmt_power`, `nz`). Kept separate from round-dashboard so the
  two tracks evolve independently.
- **New primitive:** `draw_stacked_ring(it, r_in, r_out, segs[], n, power_max, track)` —
  same sweep/scale as `draw_ring_gauge`, coloured per segment.
- Lambda reads the measured sensors, builds the segments, and draws: bezel (or green
  BOOT-press / red presence ring) → outer generation ring → inner consumption ring → the two
  ring value labels + connector arcs → weather icon → centre clock → home/Tesla battery
  sub-dials → seconds tick. Thin lambda; geometry/theme in the header.

**Status:** `make build` compiles clean (RAM 34%, Flash 59%). Not yet flashed to hardware;
entity values unverified against live HA.

## Open questions

1. Exact dark/light **hex** for the colours (amber + blue/green/purple/orange/dark-red + the green/red rings).
2. Grid↔battery flows (grid charging / battery export) — keep **off**?
3. `power_max` full-scale value (≥ peak PV and peak load).

*Resolved:* PV/load are live W; grid & battery are all measured non-negative sensors (no
signs, no derivation); centre = clock; bezel = Swiss ticks, green on BOOT press, red on
presence; entity IDs known (BYD + Fronius SolarNet + Tesla `kitt_battery`).
