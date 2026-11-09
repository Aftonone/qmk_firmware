# QMK Firmware — Sofle fork

Personal fork of [qmk/qmk_firmware](https://github.com/qmk/qmk_firmware) (`upstream` remote; `origin` is the personal fork). Work happens on the `sofle` branch; to update, `git fetch upstream && git merge upstream/master` there. Ignore the hundreds of other keyboards in `keyboards/` — only the Sofle matters.

## Scope

- Keyboard: `keyboards/sofle/rev1` (Sofle v1/v2/RGB). `keyboards/sofle/keyhive` is a different variant (Keyhive PCBs), and `keyboards/sofle_choc` is a different keyboard entirely — don't conflate them.
- Keymap: `keyboards/sofle/keymaps/Aftonone/` — `keymap.c`, `config.h`, `rules.mk`, `autocorrect_data.h`.
- Keyboard-level code (OLED display, encoders, hand swap): `keyboards/sofle/sofle.c`.
- `keyboards/sofle/sofle.c_BCK` is a personal backup — do not edit, rename, or commit it.

## Build & flash (critical)

`rev1/keyboard.json` declares a Pro Micro (atmega32u4), but the actual PCB is an **Elite Pi (RP2040)**. A plain `make sofle/rev1:Aftonone` builds AVR firmware (`.hex`) that **cannot be flashed to the board**. Always build with the converter, which produces the flashable `.uf2`:

```sh
qmk compile -kb sofle/rev1 -km Aftonone -e CONVERT_TO=elite_pi -j 0
```

- Output: `.build/sofle_rev1_Aftonone_elite_pi.uf2` (root-level `.uf2`/`.hex` files are git-ignored build output).
- Flashing is manual: put the board's half in BOOTSEL mode (double-tap the reset button), then copy the `.uf2` to the mounted RPI-RP2 volume. Each half is flashed individually. (`make sofle/rev1:Aftonone:flash CONVERT_TO=elite_pi` does this via `uf2conv --wait --deploy`; `:uf2-split-left` / `:uf2-split-right` additionally set `INIT_EE_HANDS_LEFT/RIGHT`.)
- Converter: `platforms/chibios/converters/promicro_to_elite_pi` → `promicro_to_rp2040_ce`. Do not "fix" `development_board` in `rev1/keyboard.json`; the converter is the intended path.

## Keymap architecture

- Layers (`enum sofle_layers`): `_QWERTY`, `_COLEMAK`, `_GAME` (base layouts, switched via `PDF()` keycodes `KC_QWERTY`/`KC_COLEMAK`/`KC_GAME`, stored in EEPROM); `_LOWER` (3), `_RAISE` (4), `_ADJUST` (5) wired as tri-layer; `_MOUSE` (6, mousekeys).
- `TAPPING_TERM` is intentionally 200 in `config.h` — the comment says lower it to 175 to use the dot-space tap dance. Don't "clean this up".
- Word-nav custom keycodes (`KC_PRVWD`/`KC_NXTWD`/`KC_LSTRT`/`KC_LEND`) in `process_record_user` are Mac/Linux aware via `keymap_config.swap_lctl_lgui` — keep them in sync with the Mac-mode toggle when touching modifier behavior.
- Encoders (logic in `sofle.c`): left = volume up/down/mute (MPRV/MNXT on the mouse layer), right = PGUP/PGDN.
- OLED: both halves render from `oled_task_kb` in `sofle.c` — master half shows the status screen (`print_status_narrow`), slave shows the layout/layer/WPM screen (`print_slave_status_narrow`).
- **OLED line-cost gotcha**: the panel is 128×32 (driver default — `OLED_DISPLAY_128X64` is defined nowhere), so rotated 270° it's **16 lines × 5 chars**. A word that exactly fills a line (5 chars) eats an extra blank line when written with `oled_write_ln_P` (the driver's page-advance clears the line the cursor already wrapped onto); 3–4 char words cost only 1 line unless they carry an explicit `\n`. Every screen must stay the same line count — otherwise text below shifts up and the last line stays stale (the framebuffer is only cleared at init). See the `LINE COST` comment in `sofle.c`.
- `config.h` also holds encoder pins, OLED fade/timeout, split sync settings, caps-word, auto-shift, and mousekeys tuning.

## Conventions & gotchas

- `qmk` CLI v1.2.0 is at `~/.local/bin/qmk`; `-e VAR=value` passes make variables to the build.
- Unit tests: `qmk test` — only relevant when touching `quantum/`, `drivers/`, or `tmk_core/`. Keymap work is validated by compiling.
- After merging `upstream/master`, run `git submodule update --init --recursive` **before** building — upstream regularly bumps the `lib/chibios` pointers, and a stale submodule checkout fails with an opaque `#error "obsolete or unknown configuration file"` in `lib/chibios/os/hal/include/hal.h`.
- `.github/copilot-instructions.md` holds upstream QMK PR rules (lowercase names, license headers, pristine default keymaps). Only relevant if a change is ever PR'd to `upstream` — e.g. the keymap folder name `Aftonone` would need to be renamed for a PR.
