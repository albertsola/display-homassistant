# Dashboard Solar

A round energy dashboard for a small watch-face style display, driven by Home Assistant.
It shows a home solar system at a glance — **solar, battery, grid, and home usage** — plus
**where the energy is flowing** right now.

It comes in two forms that share the same look:

- **ESPHome firmware** for a physical round display (`dashboard-solar.yaml` + `dashboard_solar.h`).
- **A Home Assistant custom card** (`dashboard-solar-card.js`) that renders the same display
  inside the HA dashboard.

## What the screen shows

Three concentric rings around a central clock and weather readout:

- **Outer ring — where power comes from (sources).** Solar generation, the battery
  discharging to the house, and grid import.
- **Inner ring — what power is spent on (usage).** House consumption, home-battery charging,
  EV charging, and anything exported to the grid.
- **Inner split ring — battery levels.** Home battery on the left, electric vehicle on the
  right.

The centre shows the current weather (icon, temperature, humidity), the date, and the time.
The outer bezel turns **red** when presence is detected.

Both rings share one power scale, so segment lengths are directly comparable, and the two
rings balance: everything coming in (solar + import + battery discharge) equals everything
going out (house + battery charge + EV + export).

### Ring colours

**Outer ring — sources**

| Colour | Meaning |
|--------|---------|
| 🟡 yellow | solar generation |
| 🟢 green | battery discharging to the house |
| 🟥 red | imported from the grid |

**Inner ring — usage**

| Colour | Meaning |
|--------|---------|
| 🔵 blue | house consumption (excluding the EV) |
| 🟢 green | charging the home battery |
| 🟣 purple | charging the EV |
| 🟥 red | exported to the grid |

## Home Assistant entities

Every value is a live, measured sensor — nothing is derived from sign conventions.

| Value | Entity |
|-------|--------|
| Solar production | `sensor.powermon_totalsolar` |
| House usage | `sensor.consum_total` |
| Home battery level | `sensor.byd_battery_box_premium_hv_state_of_charge` |
| Battery charging | `sensor.solarnet_power_battery_charge` |
| Battery discharging | `sensor.solarnet_power_battery_discharge` |
| Grid export | `sensor.solarnet_power_grid_export` |
| Grid import | `sensor.solarnet_power_grid_import` |
| EV charging | `sensor.kitt_charger_power` |
| EV battery level | `sensor.kitt_battery` |
| Presence | `binary_sensor.cam_entrada_moviment_3` |
| Weather | `weather.ivallm3` |

## The physical display (ESPHome)

**Hardware:** an ESP32-2424S012N — a round 1.28" GC9A01 display (240×240, ESP32-C3, USB-C, no
touch). Printed enclosures live in `case/`. The only physical input is the BOOT button.

This folder is self-contained. Build from inside it with Docker:

```bash
cd dashboard-solar
cp secrets.yaml.sample secrets.yaml   # fill in your Wi-Fi
make build                            # or: make run device=<ip>
```

- `dashboard-solar.yaml` — your device config (name, Wi-Fi, API key). It imports the shared
  package below.
- `dashboard-solar-package.yaml` — the reusable design (display, fonts, and the drawing logic),
  with the entities set as overridable defaults.
- `dashboard_solar.h` — the drawing helpers.
- `example-device.yaml` — a template for adopting the design on someone else's system: they
  override just the entities and supply their own secrets, importing the package over GitHub.

**Controls exposed to HA:** dark mode, screen on/off, brightness, rotation, and the shared
power scale. The BOOT button is published as a sensor.

## The Home Assistant card

`dashboard-solar-card.js` renders the same display inside the HA frontend from live entity
states. It's a compact, resizable widget on Sections dashboards, with a visual editor for
picking entities and choosing a light/dark theme.

**Install** — add a dashboard resource (Settings → Dashboards → ⋮ → Resources), type
**JavaScript module**:

```
https://cdn.jsdelivr.net/gh/albertsola/display-homassistant@main/dashboard-solar/dashboard-solar-card.js
```

jsDelivr caches branch URLs, so after an update either pin `@<tag-or-commit>` in the URL or
purge the cache once at
`https://purge.jsdelivr.net/gh/albertsola/display-homassistant@main/dashboard-solar/dashboard-solar-card.js`.

You can also host the file locally (`/config/www/` → `/local/…`) or install it through HACS.

**Add the card** — all entity keys are optional and default to the entities above:

```yaml
type: custom:dashboard-solar-card
theme: dark          # dark | light | auto (auto follows the HA theme)
power_max: 8000      # ring full-scale in W
# pv: sensor.powermon_totalsolar
# load: sensor.consum_total
# ...override any entity
```

## Simulator

`simulator.html` (and `simulator-ca.html`, in Catalan) reproduce the display in the browser
with sliders and example scenarios — handy for tuning colours and layout without hardware.
