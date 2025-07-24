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

/* Charybdis-specific features. */
#define CHARYBDIS_CONFIG_SYNC

#ifdef POINTING_DEVICE_ENABLE
    #define CHARYBDIS_DRAGSCROLL_ENABLE
    #define CHARYBDIS_SNIPING_ENABLE
// Automatically enable the pointer layer when moving the trackball.  See also:
    /* #define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS 1000 */
    /* #define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD 2 */
    /* #define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE */
#endif // POINTING_DEVICE_ENABLE

/* #define PERMISSIVE_HOLD */
#define TAPPING_TERM 180
#define TAPPING_TERM_PER_KEY
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD
