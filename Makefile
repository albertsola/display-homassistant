# ESPHome via Docker
# Usage:
#   make run                                        # OTA-flash the default config
#   make run file=battery-solar-display.yaml        # OTA-flash a specific config
#   make run file=xxx.yaml device=192.168.98.31     # force the OTA target IP
#   make build file=xxx.yaml                         # compile only (no upload)
#   make logs  file=xxx.yaml                         # stream device logs
#   make clean file=xxx.yaml                         # clear build files
#   make shell                                       # shell inside the container
#
# Case renders (needs local OpenSCAD, not Docker):
#   make render                                      # body, bezel, pcb + fit, all angles
#   make render parts="bezel jig"                    # choose which parts
#   make render parts=section                        # cutaway (with PCB seated)
#
# Case STL export (print-ready, needs local OpenSCAD):
#   make stl                                         # body + bezel -> case/flat_case/stl/*.stl
#   make render case_dir=case/other_case             # build a different case variant
#   make stl stl_fn=240                              # finer facets for the final export

file   ?= round-dashboard.yaml
device ?=

DC = docker compose run --rm esphome

# --- Case (OpenSCAD) --- override `case_dir` to build a different case variant.
case_dir ?= case/flat_case
scad     ?= $(case_dir)/round-dashboard-case.scad
parts    ?= body bezel pcb fit   # separated parts + the fitted assembly
img_dir  ?= $(case_dir)/renders
img_size ?= 1000,1000

# STL export (print orientation). friendly-name:part — the bezel is exported FLIPPED
# (flat show-face down on the bed) via the scad's `bezel_print` branch.
stl_dir   ?= $(case_dir)/stl
stl_fn    ?= 180                          # export facets (finer than the preview $fn)
stl_parts ?= body:body bezel:bezel_print

# On macOS the Homebrew symlink (/opt/homebrew/bin/openscad -> the .app) breaks
# Qt's platform-plugin lookup — Qt resolves plugins relative to the path it was
# launched with, so it never finds Contents/Resources/qt.conf and aborts with
#   could not find or load the Qt platform plugin "cocoa" in ""
# Call the binary inside the bundle directly. Override with: make render OPENSCAD=...
OPENSCAD ?= $(firstword $(wildcard /Applications/OpenSCAD*.app/Contents/MacOS/OpenSCAD))
ifeq ($(strip $(OPENSCAD)),)
OPENSCAD := openscad
endif
# angle = name:rotx,roty,rotz  (OpenSCAD gimbal; --viewall handles distance)
ANGLES = iso:55,0,25 front:90,0,0 side:90,0,90 back:290,0,25 top:0,0,0

.PHONY: run build logs clean shell render stl

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

# Render each part in $(parts) from every angle -> $(img_dir)/<part>-<angle>.png
render:
	@mkdir -p $(img_dir)
	@for p in $(parts); do \
		for a in $(ANGLES); do \
			name=$${a%%:*}; rot=$${a#*:}; \
			out=$(img_dir)/$$p-$$name.png; \
			echo "  render $$out"; \
			$(OPENSCAD) -q -o $$out --imgsize=$(img_size) --viewall --autocenter \
				--camera=0,0,0,$$rot,0 -D 'part="'$$p'"' $(scad) || exit 1; \
		done; \
	done
	@echo "Done -> $(img_dir)/"

# Export each printable part -> $(stl_dir)/round-dashboard-<name>.stl (print orientation,
# no preview tilt). Manifold check is printed by OpenSCAD ("Status: NoError").
stl:
	@mkdir -p $(stl_dir)
	@for p in $(stl_parts); do \
		name=$${p%%:*}; pv=$${p#*:}; \
		out=$(stl_dir)/round-dashboard-$$name.stl; \
		echo "  export $$out"; \
		$(OPENSCAD) -o $$out -D 'part="'$$pv'"' -D 'tilt_deg=0' -D '$$fn=$(stl_fn)' $(scad) || exit 1; \
	done
	@echo "Done -> $(stl_dir)/"
