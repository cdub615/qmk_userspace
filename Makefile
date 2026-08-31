.SILENT:

MAKEFLAGS += --no-print-directory

QMK_USERSPACE := $(patsubst %/,%,$(dir $(shell realpath "$(lastword $(MAKEFILE_LIST))")))
ifeq ($(QMK_USERSPACE),)
    QMK_USERSPACE := $(shell pwd)
endif

# Targets that don't need a qmk_firmware checkout.
LOCAL_GOALS := keymap-svg flash flash-both fetch-firmware

ifeq ($(filter $(LOCAL_GOALS),$(MAKECMDGOALS)),)
# qmk 1.2.0 appends a " (config)" source annotation to -ro output; strip it.
QMK_FIRMWARE_ROOT = $(shell qmk config -ro user.qmk_home | cut -d= -f2- | sed -e 's/ .*$$//' -e 's@^None$$@@g')
ifeq ($(QMK_FIRMWARE_ROOT),)
    $(error Cannot determine qmk_firmware location. `qmk config -ro user.qmk_home` is not set)
endif
endif

# ---------------------------------------------------------------------------
# Keymap diagram
#
# Regenerates the visual reference from keymap.yaml. Edit keymap.yaml by hand
# to match keymap.c, then run `make keymap-svg`.
# Requires `uv` (for keymap-drawer) and `rsvg-convert` (librsvg).
# ---------------------------------------------------------------------------
CDUB_KEYMAP := keyboards/bastardkb/charybdis/3x5/keymaps/cdub

.PHONY: keymap-svg
keymap-svg:
	uvx --from keymap-drawer keymap draw $(CDUB_KEYMAP)/keymap.yaml > $(CDUB_KEYMAP)/keymap.svg
	rsvg-convert -z 2 $(CDUB_KEYMAP)/keymap.svg -o $(CDUB_KEYMAP)/keymap.png
	echo "wrote $(CDUB_KEYMAP)/keymap.svg and keymap.png"

# ---------------------------------------------------------------------------
# Flashing
#
# Fetches the latest CI-built firmware from the GitHub release, verifies it is
# a valid RP2040 UF2, waits for the bootloader drive, and writes it.
#   make flash          one half
#   make flash-both     both halves, prompting between them
#   make fetch-firmware download + verify only
# ---------------------------------------------------------------------------
.PHONY: flash flash-both fetch-firmware
flash:
	util/flash.sh

flash-both:
	util/flash.sh --both

fetch-firmware:
	util/flash.sh --fetch

%:
	+$(MAKE) -C $(QMK_FIRMWARE_ROOT) $(MAKECMDGOALS) QMK_USERSPACE=$(QMK_USERSPACE)
