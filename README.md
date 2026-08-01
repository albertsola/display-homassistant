# Round Dashboard

An ESPHome "digital Swiss-watch" dashboard for a round 1.28" display, driven by
Home Assistant. Shows electricity + gas usage as concentric gauges, temperature
and humidity sub-dials, a live clock/date, and a presence alert — with theme,
brightness, rotation and gauge ranges all controllable from Home Assistant.

- **Config:** `round-dashboard.yaml` (ESPHome node name: `round-dashboard`)
- **Device:** ESP32-2424S012**N** — round 1.28" **GC9A01** panel, **240×240**, ESP32-C3
- **Device IP:** `10.12.13.192` (keep stable via a DHCP reservation for MAC `F0:F5:BD:E9:C4:10`)

## What's on screen

```
        ┌ outer bezel = 60 ticks; the current SECOND is a red tick ┐
        │      (turns SOLID RED while presence is detected)         │
        │  ┌ Electricity ring (outer) ── Gas ring (inner) ┐         │
        │  │              Sat, 1 Aug                       │        │
        │  │              12:34:56                          │       │
        │  │           (TEMP)   (HUM)                       │       │
        │  │            4.2 m3   ← gas value                │       │
        │  └             3200 W  ← electricity value ───────┘       │
        └───────────────────────────────────────────────────────────┘
```

- **Electricity** (outer) and **Gas** (inner) rings — segmented 3px ticks, filled to the value.
- **Temperature / Humidity** sub-dials below the clock (12 ticks each, 3px filled).
- **Date + time** in the centre; the outer bezel doubles as a seconds hand.
- **Presence:** when the presence sensor is on, the outer ring goes solid red.

## Home Assistant entities it reads

Set in the `substitutions:` block of `round-dashboard.yaml`:

| Screen field | Entity |
|--------------|--------|
| Electricity  | `sensor.octopus_energy_electricity_..._current_demand` |
| Gas          | `sensor.octopus_energy_gas_..._current_accumulative_consumption_m3` |
| Temperature  | `sensor.th_outdoors_temperature_2` |
| Humidity     | `sensor.th_outdoors_humidity_2` |
| Presence     | `binary_sensor.drive_person` |

> `..._current_demand` (real-time electricity) only exists with an **Octopus Home Mini**.

## Controls it exposes to Home Assistant

- **Dark Mode** (switch) — dark/light theme. Also toggled by tapping the screen (touch variant only).
- **Screen** (switch) — display on/off.
- **Brightness** (number, 0–100%) — backlight level, independent of theme.
- **Screen Rotation** (select) — 0 / 90 / 180 / 270°.
- **Gauge min/max** (numbers) — adjustable range for all four gauges.

**On boot:** Screen and Dark Mode always start **on**; Brightness defaults to 100%
(persisted); Rotation defaults to 180° (persisted). HA can override any of these at runtime.

## Hardware pins (ESP32-2424S012)

| Function | Pin | Function | Pin |
|----------|-----|----------|-----|
| SPI CLK  | GPIO6  | Display CS | GPIO10 |
| SPI MOSI | GPIO7  | Display DC | GPIO2  |
| Backlight (LEDC) | GPIO3 | Display RST | GPIO1 |
| Touch I²C SDA | GPIO4 | Touch INT | GPIO0 |
| Touch I²C SCL | GPIO5 | | |

Touch (CST816) exists only on the **"C"** board variant; the **"N"** variant has no touch.

## Build & flash (Docker, no local ESPHome)

Everything runs through Docker via the `Makefile` — no Python/ESPHome install needed.

1. Create `secrets.yaml` with your WiFi:
   ```yaml
   wifi_ssid: "YOUR_SSID"
   wifi_password: "YOUR_PASSWORD"
   ```
2. Commands:
   ```bash
   make build                       # compile only (no device needed)
   make run                          # compile + OTA-flash round-dashboard.yaml
   make run device=10.12.13.192      # OTA by IP (needed: mDNS often can't resolve from Docker)
   make logs device=10.12.13.192     # stream device logs
   make clean                        # clear build files
   make shell                        # shell inside the ESPHome container
   ```
   `file=<name>.yaml` overrides the default config; `device=<ip>` targets a specific address.

> The **first ever** flash must be over USB (not possible through Docker on macOS) — use the
> ESPHome web flasher or native ESPHome once. After that it's all OTA.

## Connect it to Home Assistant

mDNS (`.local`) often doesn't reach Home Assistant on this network, so add the device **by IP**:

- **Settings → Devices & Services → Add Integration → ESPHome**
- Host `10.12.13.192`, port `6053`
- Encryption key: the `api: → encryption: → key` value in `round-dashboard.yaml`

Only once the ESPHome **integration** is connected do the sensors populate and the controls appear.

## Files

| File | Purpose |
|------|---------|
| `round-dashboard.yaml` | the ESPHome config (device + watch-face lambda) |
| `Makefile` | build/flash/log commands (wraps `docker compose`) |
| `docker-compose.yml` | ESPHome container used by the Makefile |
| `secrets.yaml` | WiFi credentials (gitignored) |
