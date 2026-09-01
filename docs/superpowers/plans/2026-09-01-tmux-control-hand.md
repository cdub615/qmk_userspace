# tmux Control Hand Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the Media layer's dead left hand into a fifteen-key tmux control cluster, operated cross-hand while the right thumb holds `Bspc`.

**Architecture:** Twelve of the fifteen keys are plain mod-wrapped keycodes (`LALT`, `LCA`, `LSA`, `LCTL`) requiring no code at all -- the same form as the existing `LCA(KC_DELETE)` on the Symbols layer. Only the three true tmux-prefix commands (new window, zoom, detach) need a `process_record_user()` handler, which taps the prefix then the letter. Media's right hand, both thumb clusters, and every other layer are untouched.

**Tech Stack:** QMK (vendor fork `Bastardkb/bastardkb-qmk` @ `main`), RP2040, `keymap-drawer` via `uvx` for the diagram.

**Spec:** `docs/superpowers/specs/2026-08-31-tmux-layer-design.md`
**Beads epic:** `qmk-qm8`

---

## A note on testing

This repo has no unit-test harness, and a keymap cannot be meaningfully unit
tested. The automated gate is therefore **the compiler plus structural `grep`
assertions on `keymap.c`**, and the behavioural gate is **Task 5's live
verification against a running tmux session**. Do not invent a test framework;
run the greps exactly as written and compare the counts.

## The single-dispatch invariant (read before Task 1)

`process_record_user()` is reachable by **two** paths in this build:

1. `quantum.c:354` calls `process_record_modules()`, which calls
   `process_record_bk_pointing_device()`, which at
   `modules/bastardkb/bk_pointing_device/bk_pointing_device.c:397` calls
   `process_record_user()` directly. Modules are not supposed to do this; it is
   a bug in the vendor module.
2. `quantum.c:355` then calls `process_record_kb()`, whose weak default at
   `quantum.c:185` also calls `process_record_user()`.

So a handler that returns `true` runs **twice per key event**. A macro written
that way would send the tmux prefix twice.

The fix is structural, not defensive: **every custom-keycode branch must return
`false`.** Returning `false` makes `process_record_bk_pointing_device()` return
`false` at its line 398, which makes `process_record_modules()` return `false`,
which short-circuits the `&&` at `quantum.c:354` so `process_record_kb()` is
never reached. One dispatch, exactly once.

This invariant is asserted by grep in Task 1, Step 5.

## File structure

| File | Responsibility | Task |
|---|---|---|
| `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/config.h` | Add `TAP_CODE_DELAY` | 1 |
| `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c` | Keycode aliases, custom keycode enum, `process_record_user()` | 1 |
| `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c` | `LAYOUT_LAYER_MEDIA` left half | 2 |
| `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.yaml` | Media layer diagram source (hand-maintained) | 3 |
| `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/readme.md` | Document the layer's new role and the argos constraint | 4 |

---

### Task 1: Add the tmux keycode machinery

**Beads:** `qmk-qm8.1`

**Files:**
- Modify: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/config.h`
- Modify: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c`

- [ ] **Step 1: Add `TAP_CODE_DELAY` to `config.h`**

QMK defaults `TAP_CODE_DELAY` to `0` (`quantum/action.h:32`), which registers and
unregisters a keycode with no gap between them. The three macros send a prefix
immediately followed by a letter, and tmux needs to see those as two distinct
events. Nothing else in this keymap calls `tap_code`/`tap_code16` today, so this
setting affects only the new macros.

Append to `config.h`, after the `AUTO_MOUSE_DEFAULT_LAYER` block:

```c
/* Gap between the register and unregister of a tap_code16() call.
 *
 * QMK defaults this to 0. The tmux macros in keymap.c send the prefix and then
 * a letter back to back, and a zero-length gap is unreliable across that
 * boundary. Nothing else in this keymap taps codes programmatically. */
#define TAP_CODE_DELAY 10
```

- [ ] **Step 2: Add the keycode aliases to `keymap.c`**

Insert directly after the existing `#define _L_PTR(KC) LT(LAYER_POINTER, KC)`
line:

```c
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
```

- [ ] **Step 3: Add the custom keycode enum**

Insert immediately after the alias block from Step 2:

```c
/* The three commands that genuinely need the tmux prefix.
 *
 * QK_USER is the correct base with VIA enabled; the keyboard-level QK_KB range
 * is claimed by VIA and by the argos module. */
enum tmux_keycodes {
    TMX_NEW = QK_USER,  // prefix c -- new window
    TMX_ZOOM,           // prefix z -- toggle pane zoom
    TMX_DTCH,           // prefix d -- detach session
};
```

- [ ] **Step 4: Add `process_record_user()`**

Insert immediately after the existing `keyboard_post_init_user()` function and
before `const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS]`:

```c
/**
 * \brief Send a tmux prefix followed by a command letter.
 *
 * Every branch returns false, and that is load-bearing -- see the
 * single-dispatch invariant in
 * docs/superpowers/plans/2026-09-01-tmux-control-hand.md. Returning true here
 * would fire each macro twice, because bk_pointing_device.c calls
 * process_record_user() itself in addition to the normal process_record_kb()
 * path.
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
            return true;
    }

    if (record->event.pressed) {
        tap_code16(TMX_PFX);
        tap_code16(command);
    }
    return false;
}
```

- [ ] **Step 5: Assert the single-dispatch invariant**

Run:

```bash
cd /home/cdub/projects/qmk_userspace
awk '/^bool process_record_user/,/^}/' \
  keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c | grep -c 'return true'
```

Expected output: `1` -- the single `default:` branch. Any other count means a
custom keycode branch can return `true`, which breaks the invariant and will
double-send the prefix.

- [ ] **Step 6: Build**

Run:

```bash
cd /home/cdub/projects/qmk_userspace
make bastardkb/charybdis/3x5/splinktegrated_rev1:cdub
```

Expected: ends with `[OK]` and writes a `.uf2`. There must be no
`duplicate symbol` error (which would mean the argos module already defines
`process_record_user`) and no warning mentioning `QK_USER` or keycode ranges.

- [ ] **Step 7: Commit**

```bash
cd /home/cdub/projects/qmk_userspace
git add keyboards/bastardkb/charybdis/3x5/keymaps/cdub/config.h \
        keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c
git commit -m "keymap: add tmux keycode aliases and prefix macros

Twelve plain mod-wrapped aliases for the prefix-free Alt and Ctrl+Alt
bindings in the live tmux config, plus three custom keycodes for the
commands that do need the prefix.

Every branch of process_record_user() returns false on purpose:
bk_pointing_device.c calls process_record_user() from inside its own
module hook, so a branch returning true is dispatched twice and would
send the prefix twice.

Refs qmk-qm8.1"
bd close qmk-qm8.1
```

---

### Task 2: Reclaim the Media layer's left hand

**Beads:** `qmk-qm8.2` (blocked by `qmk-qm8.1`)

**Files:**
- Modify: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c` (the `LAYOUT_LAYER_MEDIA` macro)

- [ ] **Step 1: Record the "before" counts**

Run:

```bash
cd /home/cdub/projects/qmk_userspace/keyboards/bastardkb/charybdis/3x5/keymaps/cdub
for k in QK_BOOT EE_CLR KC_MPRV RM_TOGG; do printf "%s: " "$k"; grep -o "$k" keymap.c | wc -l; done
```

Expected output:

```
QK_BOOT: 1
EE_CLR: 4
KC_MPRV: 2
RM_TOGG: 2
```

- [ ] **Step 2: Replace the `LAYOUT_LAYER_MEDIA` body**

Replace the whole existing macro (the five lines beginning
`#define LAYOUT_LAYER_MEDIA`) with:

```c
#define LAYOUT_LAYER_MEDIA                                                                    \
    TMX_WPRV, TMX_WNXT, TMX_SPRV, TMX_SNXT,  TMX_NEW, XXXXXXX, RM_PREV, RM_TOGG, RM_NEXT, XXXXXXX, \
      TMX_PL,   TMX_PD,   TMX_PU,   TMX_PR,  TMX_PFX, KC_MPRV, KC_VOLD, KC_MUTE, KC_VOLU, KC_MNXT, \
    TMX_SPLD, TMX_SPLR, TMX_KILL, TMX_ZOOM, TMX_DTCH, XXXXXXX,  EE_CLR, KC_PAUS, XXXXXXX, QK_BOOT, \
                      KC_MSTP, KC_MPLY, KC_MSTP, _______, KC_MPLY
```

Also replace the doc comment above it, which still describes the symmetric
layout, with:

```c
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
```

- [ ] **Step 3: Assert the counts changed exactly as expected**

Run the same loop as Step 1:

```bash
cd /home/cdub/projects/qmk_userspace/keyboards/bastardkb/charybdis/3x5/keymaps/cdub
for k in QK_BOOT EE_CLR KC_MPRV RM_TOGG; do printf "%s: " "$k"; grep -o "$k" keymap.c | wc -l; done
```

Expected output:

```
QK_BOOT: 1
EE_CLR: 3
KC_MPRV: 1
RM_TOGG: 1
```

`QK_BOOT` must stay at `1`. `EE_CLR` drops to `3` because Media's left-hand copy
is gone while its right-hand copy and both Pointer-layer copies remain.
`KC_MPRV` and `RM_TOGG` drop to `1` each -- the surviving right-hand copies.

- [ ] **Step 4: Confirm no other layer was touched**

Run:

```bash
cd /home/cdub/projects/qmk_userspace
git diff -- keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c
```

Expected: every changed hunk falls inside the `LAYOUT_LAYER_MEDIA` macro or its
doc comment. If a hunk touches `LAYOUT_LAYER_BASE`, `LAYOUT_LAYER_FUNCTION`,
`LAYOUT_LAYER_POINTER`, `LAYOUT_LAYER_NUMERAL`, `LAYOUT_LAYER_SYMBOLS`, or the
`keymaps[]` array, revert it -- those are out of scope.

- [ ] **Step 5: Build**

```bash
cd /home/cdub/projects/qmk_userspace
make bastardkb/charybdis/3x5/splinktegrated_rev1:cdub
```

Expected: ends with `[OK]`.

- [ ] **Step 6: Commit**

```bash
cd /home/cdub/projects/qmk_userspace
git add keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c
git commit -m "keymap: put a tmux control hand on the Media layer

Media's left hand mirrored its own right hand exactly, to serve
left-handed trackball builds. The trackball on this board is on the
right, so those fifteen keys were dead weight.

They now carry tmux: pane selection on the home row, window and session
movement on the top row, and the structural commands -- splits, kill,
zoom, detach -- on the bottom row, furthest from an accidental press.

The right hand, both thumb clusters and all other layers are unchanged.

Refs qmk-qm8.2"
bd close qmk-qm8.2
```

---

### Task 3: Sync `keymap.yaml` and regenerate the diagram

**Beads:** `qmk-qm8.3` (blocked by `qmk-qm8.2`)

`keymap.yaml` is maintained by hand and is the source for the rendered diagram.
It is not generated from `keymap.c`, so it drifts unless updated deliberately.

**Files:**
- Modify: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.yaml`
- Regenerate: `keymap.svg`, `keymap.png`

- [ ] **Step 1: Replace the Media layer's three key rows**

Find the `Media:` block under `layers:`. Leave its fourth (thumb) row alone and
replace the three key rows with:

```yaml
    Media:
        - [Win ←, Win →, Ses ↑, Ses ↓, New win,
           '', RGB Prev, RGB Tog, RGB Next, '']
        - [Pane ←, Pane ↓, Pane ↑, Pane →, tmux pfx,
           Prev, 'Vol −', Mute, 'Vol +', Next]
        - [Split ↓, Split →, Kill pane, Zoom, Detach,
           '', EE Clr, Pause, '', Boot]
```

- [ ] **Step 2: Regenerate**

```bash
cd /home/cdub/projects/qmk_userspace
make keymap-svg
```

Expected: prints `wrote .../keymap.svg and keymap.png`. Requires `uv` and
`rsvg-convert`; if either is missing, install it rather than skipping this step,
because the committed diagram is the keymap's only visual reference.

- [ ] **Step 3: Verify the render is stable**

```bash
cd /home/cdub/projects/qmk_userspace
make keymap-svg
git diff --stat -- keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.svg
```

Expected: no further change to `keymap.svg` on the second run.

- [ ] **Step 4: Eyeball the PNG**

Open `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.png` and confirm the
Media layer's left hand reads as the tmux cluster and its right hand still reads
as media/RGB.

- [ ] **Step 5: Commit**

```bash
cd /home/cdub/projects/qmk_userspace
git add keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.yaml \
        keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.svg \
        keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.png
git commit -m "keymap: redraw the Media layer with the tmux hand

Refs qmk-qm8.3"
bd close qmk-qm8.3
```

---

### Task 4: Document the layer and the argos constraint

**Beads:** `qmk-qm8.4` (blocked by `qmk-qm8.2`)

**Files:**
- Modify: `keyboards/bastardkb/charybdis/3x5/keymaps/cdub/readme.md`

- [ ] **Step 1: Read the existing readme to match its voice and heading style**

```bash
cat keyboards/bastardkb/charybdis/3x5/keymaps/cdub/readme.md
```

- [ ] **Step 2: Add the tmux section**

Add, using whatever heading level the surrounding document uses:

```markdown
## The tmux hand (Media layer, left side)

Hold `Bspc` with the right thumb and the left hand becomes tmux control.
Pane selection sits on the home row because it is the most frequent action;
window and session movement are on the top row; the structural commands are on
the bottom row, furthest from an accidental press.

Most of these are plain mod-wrapped keycodes, not macros, because the live tmux
config binds them prefix-free (`bind -n`) to Alt and Ctrl+Alt chords. Only new
window, zoom and detach need the prefix, and those are the three custom
keycodes in `process_record_user()`.

`tmux pfx` (home row, index finger) emits `C-b` -- tmux's `prefix2` -- and
covers everything without its own key: `s`, `[`, `w`, `k`, `r`.

Note that `~/.tmux.conf` is stale and not loaded; the live config is
`~/.config/tmux/tmux.conf`.
```

- [ ] **Step 3: Add the constraints note**

```markdown
## Constraints worth knowing

- **Combos and tap dance cannot be defined here.** The argos module
  (`modules/bastardkb/argos/`) defines `combo_count()`/`combo_get()` and
  `tap_dance_count()`/`tap_dance_get()` non-weak, overriding QMK's weak
  versions, and drives both from EEPROM via the Argos configurator (16 combo
  slots, 4 keys each). Combos written in `keymap.c` are silently ignored.
  Escaping this means forking the `modules/bastardkb` submodule.
- **`TAPPING_TERM_PER_KEY` has no effect.** argos defines `get_tapping_term()`
  non-weak, so the tapping term is one global value.
- **`process_record_user()` is reachable twice per key event.**
  `bk_pointing_device.c` calls it from inside its own module hook, in addition
  to the normal `process_record_kb()` path. Handlers must return `false` to
  short-circuit the second dispatch.
```

- [ ] **Step 4: Repoint the `process_record_user()` comment at the readme**

The doc comment above `process_record_user()` in `keymap.c` currently cites
this plan document. Plan docs are per-epic and get archived once the epic
closes; the readme is the durable home. Now that Step 3 has put the constraint
there, change the comment's reference from

```
 * docs/superpowers/plans/2026-09-01-tmux-control-hand.md.
```

to

```
 * the "Constraints worth knowing" section of this keymap's readme.md.
```

- [ ] **Step 5: Commit**

```bash
cd /home/cdub/projects/qmk_userspace
git add keyboards/bastardkb/charybdis/3x5/keymaps/cdub/readme.md \
        keyboards/bastardkb/charybdis/3x5/keymaps/cdub/keymap.c
git commit -m "docs: describe the tmux hand and the argos constraints

Refs qmk-qm8.4"
bd close qmk-qm8.4
```

---

### Task 5: Flash and verify live

**Beads:** `qmk-qm8.5` (blocked by `qmk-qm8.2`)

`make flash-both` fetches the **CI-built** firmware from the GitHub release, not
the local build. Push first and wait for CI, or the flash will write the old
firmware.

- [ ] **Step 1: Push and wait for CI**

```bash
cd /home/cdub/projects/qmk_userspace
git push
gh run watch
```

Expected: the workflow completes successfully and publishes a release asset.

- [ ] **Step 2: Flash both halves**

```bash
cd /home/cdub/projects/qmk_userspace
make flash-both
```

Expected: the UF2 verifies, and each half's bootloader drive is detected and
written in turn without needing a keypress between them.

- [ ] **Step 3: Verify the twelve plain keycodes**

Open a tmux session with at least two windows, two panes and two sessions. Hold
`Bspc` with the right thumb throughout, and confirm each key:

| Key (Colemak position) | Expected |
|---|---|
| `Q` | previous window |
| `W` | next window |
| `F` | previous session |
| `P` | next session |
| `A` / `R` / `S` / `T` | select pane left / down / up / right |
| `Z` | split below, in the current pane's directory |
| `X` | split right, in the current pane's directory |
| `C` | kill pane |

- [ ] **Step 4: Verify the three macros fire exactly once**

| Key | Expected |
|---|---|
| `B` | one new window, in the current pane's directory |
| `D` | pane zoom toggles |
| `V` | session detaches |

Watch for a **double fire**: if `B` opens two windows, or `D` zooms and instantly
unzooms, the single-dispatch invariant is broken. Re-check that every branch of
`process_record_user()` returns `false` (Task 1, Step 5).

If instead a macro fires inconsistently, the fix depends on which half fails,
and the two have different causes:

- **The whole macro is intermittent** (sometimes nothing happens at all): the
  host is dropping a too-short keypress. Raise `TAP_CODE_DELAY` in `config.h`
  from `10` to `20` and reflash. `TAP_CODE_DELAY` is the duration each tap is
  *held* -- `tap_code16_delay()` is `register_code16` / `wait_ms(delay)` /
  `unregister_code16` (`quantum/quantum.c:154-158`).
- **The prefix lands but the letter is dropped**: this is the *boundary*
  between the two taps, which is still zero and which `TAP_CODE_DELAY` does
  not affect. Raising it will not help. Add an explicit gap in `keymap.c`
  instead:

  ```c
  if (record->event.pressed) {
      tap_code16(TMX_PFX);
      wait_ms(10);
      tap_code16(command);
  }
  ```

- [ ] **Step 5: Verify the raw prefix**

Hold `Bspc`, tap `G`, release, then press `s`. Expected: the tmux session tree
opens. Repeat with `[` for copy mode.

- [ ] **Step 6: Verify nothing else regressed**

- Media's right hand: volume up/down, mute, next/prev track, RGB toggle.
- `EE_CLR` still reachable on Media's right hand.
- Base layer typing, and the Function, Pointer, Numeral and Symbols layers.

- [ ] **Step 7: Close out**

```bash
cd /home/cdub/projects/qmk_userspace
bd close qmk-qm8.5
bd close qmk-qm8
```

---

## Rollback

Every task is a single commit. To back out the whole feature:

```bash
cd /home/cdub/projects/qmk_userspace
git revert --no-commit <task-4-sha> <task-3-sha> <task-2-sha> <task-1-sha>
git commit -m "Revert the tmux control hand"
```

Then push and reflash, since `make flash-both` pulls from CI.
