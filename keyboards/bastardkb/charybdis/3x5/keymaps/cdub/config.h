/**
 * Copyright 2021 Charly Delay <charly@codesink.dev> (@0xcharly)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#ifdef VIA_ENABLE
/* VIA configuration. */
#    define DYNAMIC_KEYMAP_LAYER_COUNT 6
#endif // VIA_ENABLE

#ifndef __arm__
/* Disable unused features. */
#    define NO_ACTION_ONESHOT
#endif // __arm__

/* Charybdis-specific features.
 *
 * The old CHARYBDIS_* API is gone: charybdis.h/.c no longer define
 * DRGSCRL/SNIPING/DPI_MOD, and drag-scroll + precision mode now come from the
 * bastardkb/bk_pointing_device community module (enabled in keymap.json).
 *
 * CHARYBDIS_CONFIG_SYNC must NOT be set -- it makes the keyboard's
 * post_config.h claim SPLIT_TRANSACTION_IDS_KB, which collides with the one
 * the argos module defines. Split config syncing is handled by the module now.
 */

/* #define PERMISSIVE_HOLD */
#define TAPPING_TERM 180
#define TAPPING_TERM_PER_KEY
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD

/* Required by the bastardkb/bk_pointing_device module (see upstream vendor
 * keymap config.h). LED_DPI_INDICATOR_INDEX drives the DPI indicator LED;
 * RGBLIGHT_LED_COUNT is used for its symmetric-index maths. */
#ifdef LED_DPI_INDICATOR_INDEX
#    undef LED_DPI_INDICATOR_INDEX
#endif
#define LED_DPI_INDICATOR_INDEX 0

#ifdef RGBLIGHT_LED_COUNT
#    undef RGBLIGHT_LED_COUNT
#endif
#define RGBLIGHT_LED_COUNT 36

/* Auto mouse layer.
 *
 * QMK defaults AUTO_MOUSE_DEFAULT_LAYER to 1, which here is LAYER_FUNCTION --
 * so trackball motion would raise the wrong layer, and bk_pointing_device ties
 * auto-sniping to this same layer. Point it at LAYER_POINTER.
 *
 * NOTE: this is a raw index and cannot reference the enum in keymap.c. If the
 * layer order changes, update it here too. Currently:
 *   0 BASE  1 FUNCTION  2 MEDIA  3 POINTER  4 NUMERAL  5 SYMBOLS
 */
#ifdef AUTO_MOUSE_DEFAULT_LAYER
#    undef AUTO_MOUSE_DEFAULT_LAYER
#endif
#define AUTO_MOUSE_DEFAULT_LAYER 3

/* How long each tap_code16() call holds its keycode down.
 *
 * QMK defaults this to 0, registering and unregistering in the same breath,
 * which hosts drop. tap_code16_delay() is register / wait_ms(delay) /
 * unregister (quantum/quantum.c:154-158), so this is the *hold* duration. It
 * does NOT insert a gap between consecutive taps -- the boundary between the
 * two taps in a tmux macro is still zero. Those macros depend on the hold, not
 * on any inter-tap gap; if a boundary gap is ever needed, add an explicit
 * wait_ms() between the tap_code16() calls in keymap.c.
 *
 * This is firmware-global rather than keymap-local: argos_tapdance.c:108 waits
 * TAP_CODE_DELAY on tap-dance reset, a no-op at 0 and now 10ms for any argos
 * tap dance actually configured in EEPROM. */
#define TAP_CODE_DELAY 10
