# ESPHome via Docker
# Usage:
#   make run                                        # OTA-flash the default config
#   make run file=battery-solar-display.yaml        # OTA-flash a specific config
#   make run file=xxx.yaml device=192.168.98.31     # force the OTA target IP
#   make build file=xxx.yaml                         # compile only (no upload)
#   make logs  file=xxx.yaml                         # stream device logs
#   make clean file=xxx.yaml                         # clear build files
#   make shell                                       # shell inside the container

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
