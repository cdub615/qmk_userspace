.SILENT:

MAKEFLAGS += --no-print-directory

QMK_USERSPACE := $(patsubst %/,%,$(dir $(shell realpath "$(lastword $(MAKEFILE_LIST))")))
ifeq ($(QMK_USERSPACE),)
    QMK_USERSPACE := $(shell pwd)
endif

# Documentation targets don't need qmk_firmware, so don't demand it for them.
DOC_GOALS := keymap-svg

ifeq ($(filter $(DOC_GOALS),$(MAKECMDGOALS)),)
QMK_FIRMWARE_ROOT = $(shell qmk config -ro user.qmk_home | cut -d= -f2 | sed -e 's@^None$$@@g')
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

%:
	+$(MAKE) -C $(QMK_FIRMWARE_ROOT) $(MAKECMDGOALS) QMK_USERSPACE=$(QMK_USERSPACE)
