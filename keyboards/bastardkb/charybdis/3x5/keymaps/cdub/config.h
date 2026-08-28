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
#    define DYNAMIC_KEYMAP_LAYER_COUNT 7
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
