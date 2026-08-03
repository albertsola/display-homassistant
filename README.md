# Display / Home Assistant dashboards

Firmware for round ESP32 (GC9A01) Home-Assistant display dashboards. Each dashboard is an
**independent, self-contained folder** — its own ESPHome config, drawing helpers, `Makefile`,
`docker-compose.yml`, and `secrets.yaml.sample`. Build one without touching the others.

| Folder | What |
|--------|------|
| [`round-dashboard/`](round-dashboard/) | The original "Swiss-watch" dashboard — clock, electricity/gas ring gauges, temperature/humidity sub-dials, weather, presence. |
| [`dashboard-solar/`](dashboard-solar/) | Solar-energy dashboard — solar/battery/grid/home with a stacked flow ring. See its [`README.md`](dashboard-solar/README.md) for the full design spec. |

## Build any dashboard

Each folder builds the same way (needs Docker; no local ESPHome/Python). **Run from inside
the folder:**

```bash
cd dashboard-solar                      # or: cd round-dashboard
cp secrets.yaml.sample secrets.yaml     # then fill in your Wi-Fi
make build                              # compile only (no device)
make run  device=10.12.13.50            # compile + OTA-flash (device IP)
make logs device=10.12.13.50            # stream logs
```

The first-ever flash must be over USB (not possible via Docker on macOS); everything after
is OTA. Each folder's `README.md` / `Makefile` header has the specifics.

## 3D-printed cases

Printable enclosures for the round module live under `case/` (parametric OpenSCAD; one
folder per variant). They're shared across dashboards since the board is the same.
