# Reclaim the Media layer's left hand as a tmux control hand

Date: 2026-08-31
Keymap: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/`

## Problem

Driving tmux from the Charybdis Nano is the one piece of daily friction on this
keymap. The cause is not the tmux prefix, as first assumed -- it is that the
live tmux config binds most pane and window commands prefix-free (`bind -n`) to
**Alt** and **Ctrl+Alt** chords, and those chords are expensive on a 34-key
board.

`C-M-Left` (select pane left) currently costs four simultaneous keys:

- hold `Tab` (left thumb 2) to raise Symbols, where the arrows live
- hold `C` (left bottom row) for Ctrl
- hold `X` (left bottom row) for Alt -- a second finger on the same hand that is
  already holding the thumb key
- press the arrow with the right hand

The same shape applies to `M-Enter`, `M-S-Enter`, `M-Escape`, `M-Left/Right`,
`M-Up/Down` and `M-1..9`.

### Live tmux bindings this design targets

Read from `~/.config/tmux/tmux.conf` (tmux 3.7). Prefix is `C-Space`, with
`C-b` as `prefix2`.

| Action | Live binding | Prefix? |
|---|---|---|
| Split below / right | `M-Enter` / `M-S-Enter` | no |
| Kill pane | `M-Escape` | no |
| Select pane | `C-M-Left/Down/Up/Right` | no |
| Window prev / next | `M-Left` / `M-Right` | no |
| Window 1-9 | `M-1`..`M-9` | no |
| Session prev / next | `M-Up` / `M-Down` | no |
| New window / zoom / detach / kill window | prefix `c` / `z` / `d` / `k` | yes |

Note: `~/.tmux.conf` (symlink into `dotfiles/tmux/`) sets `prefix C-a` and loads
`vim-tmux-navigator`, but is **not** loaded by tmux 3.7 -- the XDG path wins. It
is stale and was not used as a source for this design.

## Where the keys come from

The Media layer (index 2) is held by the **right** thumb (`BSP_MED`), so its
left hand is cross-hand -- the ergonomically good kind of free space. Today that
left hand is entirely dead weight: its RGB row and its
`Prev/Vol-/Mute/Vol+/Next` home row are exact duplicates of the same keys on
Media's own right hand, present only to serve left-handed trackball builds. This
board has the trackball on the right.

That yields all 15 left-hand slots.

### Rejected alternatives

- **Function layer's empty left hand (15 keys).** Function is held by the *left*
  inner thumb, which is why its F-keys sit on the right hand. Those 15 keys are
  empty by design and are same-hand with their own hold key. Left as-is.
- **A dedicated seventh Tmux layer.** All five thumb keys already carry layer
  taps, so entry would cost a free key plus a two-step reach, and
  `AUTO_MOUSE_DEFAULT_LAYER` in `config.h` is a hardcoded index that would need
  to stay in sync. More machinery than the space requires.
- **Combos.** Not available. See Constraints.

## Design

Media layer, left hand. Right hand, both thumb clusters, and every other layer
are unchanged.

```
row 1   M-Left      M-Right     M-Up        M-Down      prefix c *
row 2   C-M-Left    C-M-Down    C-M-Up      C-M-Right   C-b
row 3   M-Enter     M-S-Enter   M-Escape    prefix z *  prefix d *
```

Reading the physical Colemak-DH positions:

| Position | Keycode | tmux effect |
|---|---|---|
| `Q` | `LALT(KC_LEFT)` | previous window |
| `W` | `LALT(KC_RGHT)` | next window |
| `F` | `LALT(KC_UP)` | previous session |
| `P` | `LALT(KC_DOWN)` | next session |
| `B` | `TMX_NEW` * | new window (prefix `c`) |
| `A` | `LCA(KC_LEFT)` | select pane left |
| `R` | `LCA(KC_DOWN)` | select pane down |
| `S` | `LCA(KC_UP)` | select pane up |
| `T` | `LCA(KC_RGHT)` | select pane right |
| `G` | `TMX_PFX` | raw prefix (`C-b`) |
| `Z` | `LALT(KC_ENT)` | split below |
| `X` | `LSA(KC_ENT)` | split right |
| `C` | `LALT(KC_ESC)` | kill pane |
| `D` | `TMX_DTCH` * | detach (prefix `d`) |
| `V` | `TMX_ZOOM` * | toggle zoom (prefix `z`) |

Rationale for the row assignment: pane selection is the highest-frequency
action, so it takes the home row. Window and session movement pair up on the top
row. Structural changes -- creating and destroying panes -- sit on the bottom
row, furthest from an accidental press.

`TMX_PFX` on the home index deliberately covers everything not given a key:
`s` (session tree), `[` (copy mode), `w` (window list), `k` (kill window),
`r` (rename window). Tap it, release Media, then press the letter.

Window selection `M-1..9` is intentionally omitted -- nine slots for what
`M-Left`/`M-Right` already covers is a poor trade.

### Implementation

Three of the fifteen keys need code; the rest are plain mod-wrapped keycodes of
the same form as the existing `LCA(KC_DELETE)` on the Symbols layer.

- `TMX_PFX` is a plain keycode: `#define TMX_PFX LCTL(KC_B)`. `C-b` is chosen
  over the primary `C-Space` because it is unambiguous to emit and cannot be
  confused with an input-method toggle.
- `TMX_NEW`, `TMX_ZOOM`, `TMX_DTCH` are custom keycodes handled in
  `process_record_user()`, each sending the prefix followed by its letter, e.g.
  `SEND_STRING(SS_LCTL("b") "z")`.

The custom keycode enum must start at a base that does not collide with VIA or
with the `bk_pointing_device` module's keycode range (`0x7E00`-`0x7E07`:
`DPI_MOD` through `DRG_TOG`, in that module's `introspection.h`). `QK_USER`
(`0x7E40`) is the intended base and the actual value must be confirmed against
a clean build. Note argos claims no keycodes at all -- it works through the
combo, tap-dance and tapping-term overrides instead.

### Keys removed

Reclaiming the left hand drops one `EE_CLR`. The `EE_CLR` on Media's right hand
(right index, bottom row) remains, so the function is not lost. The single
`QK_BOOT` on Media's bottom-right pinky is untouched.

## Scope

In scope:

- `keymap.c`: rewrite `LAYOUT_LAYER_MEDIA`'s left hand, add the custom keycode
  enum and `process_record_user()`.
- `keymap.yaml`: update the Media layer by hand to match, then regenerate
  `keymap.svg` / `keymap.png` with `make keymap-svg`.
- `readme.md`: note the Media layer's new left-hand role.

Explicitly out of scope:

- Any change to the Function, Pointer, Numeral, Symbols or Base layers.
- Combos and tap dance (blocked -- see Constraints).
- The two configuration conflicts found while reading the dotfiles (below).
  They are real but live outside this repo.
- Hyprland bindings. Window management was not reported as friction; the
  existing Super+M/I/E/N focus and Super+Shift+same swap bindings stay.

## Constraints

- **Combos and tap dance cannot be defined in `keymap.c`.**
  `modules/bastardkb/argos/argos_combo.c` defines `combo_count()` and
  `combo_get()` non-weak, and `argos_tapdance.c` does the same for
  `tap_dance_count()`/`tap_dance_get()`. QMK's versions in
  `quantum/keymap_introspection.c` are `__attribute__((weak))`, so argos wins at
  link time and reads 16 combo slots (4 keys each) from EEPROM instead. Escaping
  this would mean forking the `modules/bastardkb` submodule.
- The tapping term is a single global value read from EEPROM. argos defines
  `get_tapping_term()` non-weak (`argos.c:520`), so per-key tapping terms are
  impossible, and `TAPPING_TERM` seeds EEPROM only on a board's first-ever boot
  (`argos.c:80`). **Leave `TAPPING_TERM_PER_KEY` defined** -- QMK only compiles
  `get_tapping_term()` when it is set (`action_tapping.c:34`), so removing it
  would disable argos's override rather than tidy anything up. (An earlier
  draft of this spec said the define "has no effect", which invited exactly
  that deletion.)
- `AUTO_MOUSE_DEFAULT_LAYER` in `config.h` is the hardcoded index `3`. This
  design does not reorder layers, so it needs no change.
- Auto mouse layer stays disabled in `keyboard_post_init_user()`.
- Exactly one `QK_BOOT`, on Media's bottom-right pinky.

## Acceptance criteria

1. `make bastardkb/charybdis/3x5/splinktegrated_rev1:cdub` builds clean, with no
   duplicate-symbol or keycode-range warnings.
2. Media's right hand, both thumb clusters, and all other layers are
   byte-identical in behaviour to before.
3. Held with the right thumb, each of the fifteen left-hand keys produces its
   row in the table above, verified live against a running tmux session:
   panes select, splits open below and to the right, kill pane works, windows
   and sessions cycle, zoom and detach fire, and `TMX_PFX` followed by `s` opens
   the session tree.
4. `keymap.yaml` matches `keymap.c`, and `make keymap-svg` regenerates
   `keymap.svg`/`keymap.png` without manual edits afterwards.
5. `EE_CLR` still reachable; exactly one `QK_BOOT` remains.

## Follow-ups found, not addressed here

- nvim maps `<C-Left/Down/Up/Right>` to `NvimTmuxNavigate*`, but the live tmux
  config selects panes with `C-M-arrow` and does not load `vim-tmux-navigator`
  (that plugin list is in the stale `~/.tmux.conf`). The nvim-to-tmux pane
  handoff is probably not working.
- nvim's `<A-Up/Down/Left/Right>` window-resize maps are unreachable inside
  tmux: tmux's `-n` bindings for `M-Up/Down/Left/Right` intercept them before
  the pane sees them.
- `~/.tmux.conf` is stale and unloaded; it should probably be deleted from the
  dotfiles repo.
