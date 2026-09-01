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
#include QMK_KEYBOARD_H
#include "print.h"

// Automatically enable sniping-mode on the pointer layer.
/* #define CHARYBDIS_AUTO_SNIPING_ON_LAYER LAYER_POINTER */
/* #define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS 1000 */

#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
#    include "timer.h"
#endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

enum charybdis_keymap_layers {
    LAYER_BASE = 0,
    LAYER_FUNCTION,
    LAYER_MEDIA,
    LAYER_POINTER,
    LAYER_NUMERAL,
    LAYER_SYMBOLS,
};

#ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
static uint16_t auto_pointer_layer_timer = 0;

#    ifndef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS
#        define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS 1000
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS

#    ifndef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD
#        define CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD 8
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD
#endif     // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

#define ESC_PNT LT(LAYER_POINTER, KC_ESC)
#define TAB_SYM LT(LAYER_SYMBOLS, KC_TAB)
#define ENT_FUN LT(LAYER_FUNCTION, KC_ENT)
#define BSP_MED LT(LAYER_MEDIA, KC_BSPC)
#define SPC_NUM LT(LAYER_NUMERAL, KC_SPC)
#define _L_PTR(KC) LT(LAYER_POINTER, KC)

/* tmux control hand (Media layer, left side).
 *
 * Bindings come from ~/.config/tmux/tmux.conf. Note that most of them are
 * prefix-free (`bind -n`) Alt / Ctrl+Alt chords, which is why these are plain
 * mod-wrapped keycodes rather than macros.
 *
 * TMX_PFX emits C-b, which is tmux's `prefix2`. The primary prefix is C-Space,
 * but C-b is unambiguous to emit and cannot be mistaken for an input-method
 * toggle. */
#define TMX_PFX  LCTL(KC_B)     // raw prefix, for anything without its own key
#define TMX_WPRV LALT(KC_LEFT)  // previous window
#define TMX_WNXT LALT(KC_RGHT)  // next window
#define TMX_SPRV LALT(KC_UP)    // previous session
#define TMX_SNXT LALT(KC_DOWN)  // next session
#define TMX_PL   LCA(KC_LEFT)   // select pane left
#define TMX_PD   LCA(KC_DOWN)   // select pane down
#define TMX_PU   LCA(KC_UP)     // select pane up
#define TMX_PR   LCA(KC_RGHT)   // select pane right
#define TMX_SPLD LALT(KC_ENT)   // split below
#define TMX_SPLR LSA(KC_ENT)    // split right
#define TMX_KILL LALT(KC_ESC)   // kill pane

/* The three commands that genuinely need the tmux prefix.
 *
 * QK_USER (0x7E40) is the correct base with VIA enabled. Do not use the
 * keyboard-level QK_KB range: VIA uses it, and bk_pointing_device already
 * occupies 0x7E00-0x7E07 (DPI_MOD through DRG_TOG, see that module's
 * introspection.h). argos claims no keycodes -- it works through the combo,
 * tap-dance and tapping-term overrides instead. */
enum tmux_keycodes {
    TMX_NEW = QK_USER,  // prefix c -- new window
    TMX_ZOOM,           // prefix z -- toggle pane zoom
    TMX_DTCH,           // prefix d -- detach session
};

/* #ifndef POINTING_DEVICE_ENABLE */
/* #    define DRGSCRL KC_NO */
/* #    define DPI_MOD KC_NO */
/* #    define S_D_MOD KC_NO */
/* #    define SNIPING KC_NO */
/* #endif // !POINTING_DEVICE_ENABLE */

// clang-format off
/** \brief COLEMAK DH layout (3 rows, 10 columns). */
#define LAYOUT_LAYER_BASE                                                                     \
       KC_Q,    KC_W,    KC_F,    KC_P,    KC_B,    KC_J,    KC_L,    KC_U,    KC_Y,    KC_QUOT, \
       KC_A,    KC_R,    KC_S,    KC_T,    KC_G,    KC_M,    KC_N,    KC_E,    KC_I,    KC_O, \
       KC_Z,    KC_X,    KC_C,    KC_D,    KC_V,    KC_K,    KC_H, KC_COMM,  KC_DOT, KC_SLSH, \
                      ESC_PNT, TAB_SYM, ENT_FUN, BSP_MED, SPC_NUM


/** Convenience row shorthands. */
#define _______________DEAD_HALF_ROW_______________ XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX
#define ______________BOTTOM_ROW_GACS_L______________ KC_LGUI, KC_LALT, KC_LCTL, KC_LSFT, XXXXXXX
#define ______________BOTTOM_ROW_GACS_R______________ XXXXXXX, KC_LSFT, KC_LCTL, KC_LALT, KC_LGUI

/*
 * Layers used on the Charybdis Nano.
 *
 * These layers started off heavily inspired by the Miryoku layout, but trimmed
 * down and tailored for a stock experience that is meant to be fundation for
 * further personalization.
 *
 * See https://github.com/manna-harbour/miryoku for the original layout.
 */

/**
 * \brief Function layer.
 *
 * Secondary right-hand layer has function keys mirroring the numerals on the
 * primary layer with extras on the pinkie column, plus system keys on the inner
 * column. App is on the tertiary thumb key and other thumb keys are duplicated
 * from the base layer to enable auto-repeat.
 */
#define LAYOUT_LAYER_FUNCTION                                                                 \
        _______________DEAD_HALF_ROW_______________,   KC_PSCR, KC_F7, KC_F8, KC_F9, KC_F12, \
        _______________DEAD_HALF_ROW_______________,   KC_CAPS, KC_F4, KC_F5, KC_F6, KC_F11, \
        XXXXXXX, QK_REBOOT, XXXXXXX, XXXXXXX, XXXXXXX, HYPR(KC_NO), KC_F1, KC_F2, KC_F3, KC_F10, \
                      XXXXXXX, XXXXXXX, _______, XXXXXXX, XXXXXXX
/**
 * \brief Media and tmux layer.
 *
 * Held with the right thumb (Bspc), so the left hand is cross-hand -- the
 * comfortable kind of free space. The right hand keeps media and RGB control.
 *
 * The left hand used to mirror the right one exactly, to serve left-handed
 * trackball builds. This board's trackball is on the right, so those fifteen
 * keys were dead weight and now drive tmux:
 *
 *   window prev/next, session prev/next, new window
 *   select pane left/down/up/right, raw prefix
 *   split below, split right, kill pane, zoom, detach
 *
 * TMX_PFX covers everything without its own key -- s, [, w, k, r.
 */
#define LAYOUT_LAYER_MEDIA                                                                    \
    TMX_WPRV, TMX_WNXT, TMX_SPRV, TMX_SNXT,  TMX_NEW, XXXXXXX, RM_PREV, RM_TOGG, RM_NEXT, XXXXXXX, \
      TMX_PL,   TMX_PD,   TMX_PU,   TMX_PR,  TMX_PFX, KC_MPRV, KC_VOLD, KC_MUTE, KC_VOLU, KC_MNXT, \
    TMX_SPLD, TMX_SPLR, TMX_KILL, TMX_ZOOM, TMX_DTCH, XXXXXXX,  EE_CLR, KC_PAUS, XXXXXXX, QK_BOOT, \
                      KC_MSTP, KC_MPLY, KC_MSTP, _______, KC_MPLY

/** \brief Mouse emulation and pointer functions. */
#define LAYOUT_LAYER_POINTER                                                                  \
    XXXXXXX,  EE_CLR, XXXXXXX, DPI_MOD, S_D_MOD, S_D_MOD, DPI_MOD, XXXXXXX,  EE_CLR, XXXXXXX, \
    _______,  XXXXXXX, DRGSCRL, SNIPING, XXXXXXX, XXXXXXX, MS_BTN1, MS_BTN2, MS_BTN3, _______, \
    ______________BOTTOM_ROW_GACS_L______________, MS_WHLL, MS_WHLD, MS_WHLU, MS_WHLR, KC_LGUI, \
                      _______, XXXXXXX, XXXXXXX, MS_WHLD, MS_WHLU


/**
 * \brief Numeral layout.
 *
 * Primary left-hand layer (right home thumb) is numerals and symbols. Numerals
 * are in the standard numpad locations with symbols in the remaining positions.
 * `KC_DOT` is duplicated from the base layer.
 */
#define LAYOUT_LAYER_NUMERAL                                                                  \
    KC_ESC,  XXXXXXX, LSFT(KC_9), LSFT(KC_0), LSFT(KC_MINUS),                          KC_MINUS,  KC_7,  KC_8,  KC_9, KC_0, \
    KC_TAB,  LSFT(KC_BSLS),  LSFT(KC_LBRC),  LSFT(KC_RBRC),  KC_EQUAL,           LSFT(KC_EQUAL),  KC_4,  KC_5,  KC_6, KC_ENT, \
    KC_GRAVE,  LSFT(KC_GRAVE),  KC_LBRC,  KC_RBRC,  KC_BSLS,                           XXXXXXX,  KC_1,  KC_2,  KC_3, KC_PAST, \
                                    KC_LALT, MO(LAYER_FUNCTION), KC_LSFT,      XXXXXXX, XXXXXXX

/**
 * \brief Symbols layer.
 *
 * Secondary left-hand layer has shifted symbols in the same locations to reduce
 * chording when using mods with shifted symbols. `KC_LPRN` is duplicated next to
 * `KC_RPRN`.
 */
#define LAYOUT_LAYER_SYMBOLS                                                                  \
    RSFT(KC_1), RSFT(KC_2),   RSFT(KC_3),  RSFT(KC_4), RSFT(KC_5),                  RSFT(KC_6), RSFT(KC_7), RSFT(KC_8), KC_SCLN, RSFT(KC_SCLN), \
    KC_ESC, KC_LCTL, RCS_T(KC_ENT), KC_TAB,  KC_LSFT,                                KC_LEFT, KC_DOWN, KC_UP, KC_RIGHT, KC_QUOT, \
    KC_DEL, KC_DEL, KC_DEL, LCA(KC_DELETE), KC_LGUI,                                KC_HOME, KC_PGDN, KC_PGUP, KC_END, RSFT(KC_QUOTE), \
                                    XXXXXXX, XXXXXXX, KC_TRNS,     KC_RALT, CW_TOGG

/**
 * \brief Add Home Row mod to a layout.
 *
 * Expects a 10-key per row layout.  Adds support for GACS (Gui, Alt, Ctl, Shift)
 * home row.  The layout passed in parameter must contain at least 20 keycodes.
 *
 * This is meant to be used with `LAYER_ALPHAS_QWERTY` defined above, eg.:
 *
 *     BOTTOM_ROW_MOD_GACS(LAYER_ALPHAS_QWERTY)
 */
#define _BOTTOM_ROW_MOD_GACS(                                            \
    L00, L01, L02, L03, L04, R05, R06, R07, R08, R09,                  \
    L10, L11, L12, L13, L14, R15, R16, R17, R18, R19,                  \
    L20, L21, L22, L23, L24, R25, R26, R27, R28, R29,                  \
    ...)                                                               \
      LSG_T(L00),  LSA_T(L01),  C_S_T(L02),         L03,         L04,  \
             R05,         R06,         R07,         R08,         R09,  \
             L10,         L11,         L12,         L13,         L14,  \
             R15,         R16,         R17,         R18,         R19, \
      LGUI_T(L20), LALT_T(L21), LCTL_T(L22), LSFT_T(L23),        L24,  \
             R25,  RSFT_T(R26), RCTL_T(R27), LALT_T(R28), RGUI_T(R29), \
      __VA_ARGS__
#define BOTTOM_ROW_MOD_GACS(...) _BOTTOM_ROW_MOD_GACS(__VA_ARGS__)

/**
 * \brief Add pointer layer keys to a layout.
 *
 * Expects a 10-key per row layout.  The layout passed in parameter must contain
 * at least 30 keycodes.
 *
 * This is meant to be used with `LAYER_ALPHAS_QWERTY` defined above, eg.:
 *
 *     POINTER_MOD(LAYER_ALPHAS_QWERTY)
 */
#define _POINTER_MOD(                                                  \
    L00, L01, L02, L03, L04, R05, R06, R07, R08, R09,                  \
    L10, L11, L12, L13, L14, R15, R16, R17, R18, R19,                  \
    L20, L21, L22, L23, L24, R25, R26, R27, R28, R29,                  \
    ...)                                                               \
             L00,         L01,         L02,         L03,         L04,  \
             R05,         R06,         R07,         R08,         R09,  \
             L10,         L11,         L12,         L13,         L14,  \
             R15,         R16,         R17,         R18,         R19,  \
             L20,         L21,         L22,         L23, _L_PTR(L24),  \
     _L_PTR(R25),         R26,         R27,         R28,         R29, \
      __VA_ARGS__
#define POINTER_MOD(...) _POINTER_MOD(__VA_ARGS__)

#define LAYOUT_wrapper(...) LAYOUT(__VA_ARGS__)

/**
 * \brief Force the auto mouse layer off.
 *
 * Raising the pointer layer whenever the trackball moves proved more
 * disruptive than useful. The module reads this setting from EEPROM in
 * keyboard_post_init_bk_pointing_device(), which quantum/keyboard.c runs
 * *before* keyboard_post_init_user(), so turning it off here reliably wins
 * regardless of what a previous firmware persisted.
 *
 * Guarded so it only writes EEPROM when it actually needs to change --
 * bkpd_set_auto_mouse_layer_enabled() calls write_bkpd_config_to_eeprom().
 *
 * Note this does not affect auto precision-mode, which the module gates on a
 * separate flag (auto_precision_on_mouse_layer_enabled).
 */
bool bkpd_get_auto_mouse_layer_enabled(void);
void bkpd_set_auto_mouse_layer_enabled(bool enabled);

void keyboard_post_init_user(void) {
    if (bkpd_get_auto_mouse_layer_enabled()) {
        bkpd_set_auto_mouse_layer_enabled(false);
    }
}

/**
 * \brief Send a tmux prefix followed by a command letter.
 *
 * Every branch returns false, and that is load-bearing -- see the
 * single-dispatch invariant under "Constraints worth knowing" in this
 * keymap's readme.md. Returning true here would fire each macro twice,
 * because bk_pointing_device.c calls process_record_user() itself in
 * addition to the normal process_record_kb() path.
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    uint16_t command = KC_NO;

    switch (keycode) {
        case TMX_NEW:
            command = KC_C;
            break;
        case TMX_ZOOM:
            command = KC_Z;
            break;
        case TMX_DTCH:
            command = KC_D;
            break;
        default:
            return true;  // pass-through only -- custom keycodes MUST return false
    }

    if (record->event.pressed) {
        tap_code16(TMX_PFX);
        tap_code16(command);
    }
    return false;
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [LAYER_BASE] = LAYOUT_wrapper(
    POINTER_MOD(BOTTOM_ROW_MOD_GACS(LAYOUT_LAYER_BASE))
  ),
  [LAYER_FUNCTION] = LAYOUT_wrapper(LAYOUT_LAYER_FUNCTION),
  [LAYER_MEDIA] = LAYOUT_wrapper(LAYOUT_LAYER_MEDIA),
  [LAYER_NUMERAL] = LAYOUT_wrapper(LAYOUT_LAYER_NUMERAL),
  [LAYER_POINTER] = LAYOUT_wrapper(LAYOUT_LAYER_POINTER),
  [LAYER_SYMBOLS] = LAYOUT_wrapper(LAYOUT_LAYER_SYMBOLS),
};
// clang-format on

#ifdef POINTING_DEVICE_ENABLE
#    ifdef CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    if (abs(mouse_report.x) > CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD || abs(mouse_report.y) > CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_THRESHOLD) {
        /* uprintf("Mouse movement detected: x=%d, y=%d\n", mouse_report.x, mouse_report.y); */
        if (auto_pointer_layer_timer == 0) {
            /* uprintf("Activating pointer layer\n"); */
            layer_on(LAYER_POINTER);
#        ifdef RGB_MATRIX_ENABLE
            rgb_matrix_mode_noeeprom(RGB_MATRIX_NONE);
            rgb_matrix_sethsv_noeeprom(HSV_GREEN);
#        endif // RGB_MATRIX_ENABLE
        }
        auto_pointer_layer_timer = timer_read();
    }
    return mouse_report;
}

void matrix_scan_user(void) {
    if (auto_pointer_layer_timer != 0 && TIMER_DIFF_16(timer_read(), auto_pointer_layer_timer) >= CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_TIMEOUT_MS) {
        auto_pointer_layer_timer = 0;
        layer_off(LAYER_POINTER);
#        ifdef RGB_MATRIX_ENABLE
        rgb_matrix_mode_noeeprom(RGB_MATRIX_DEFAULT_MODE);
#        endif // RGB_MATRIX_ENABLE
    }
}
#    endif // CHARYBDIS_AUTO_POINTER_LAYER_TRIGGER_ENABLE

#    ifdef CHARYBDIS_AUTO_SNIPING_ON_LAYER
layer_state_t layer_state_set_user(layer_state_t state) {
    charybdis_set_pointer_sniping_enabled(layer_state_cmp(state, CHARYBDIS_AUTO_SNIPING_ON_LAYER));
    return state;
}
#    endif // CHARYBDIS_AUTO_SNIPING_ON_LAYER
#endif     // POINTING_DEVICE_ENABLE

#ifdef RGB_MATRIX_ENABLE
// Forward-declare this helper function since it is defined in
// rgb_matrix.c.
void rgb_matrix_update_pwm_buffers(void);
#endif
