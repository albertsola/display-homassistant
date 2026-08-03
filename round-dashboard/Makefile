# Round Dashboard — self-contained ESPHome build (Docker). Run from THIS folder.
# Usage:
#   cp secrets.yaml.sample secrets.yaml   # then fill in your Wi-Fi
#   make build                            # compile only (no device)
#   make run   device=10.12.13.192        # compile + OTA-flash (device IP required)
#   make logs  device=10.12.13.192        # stream device logs
#   make clean                            # clear build files
#   make shell                            # shell inside the ESPHome container
#
# First-ever flash must be over USB (not possible via Docker on macOS); OTA after that.

file   ?= round-dashboard.yaml
device ?=

DC = docker compose run --rm esphome

.PHONY: run build logs clean shell

run:
	$(DC) run $(file) $(if $(device),--device $(device),)

build:
	$(DC) compile $(file)

logs:
	$(DC) logs $(file) $(if $(device),--device $(device),)

clean:
	$(DC) clean $(file)

shell:
	docker compose run --rm --entrypoint bash esphome
