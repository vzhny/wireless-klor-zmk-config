# KLOR ZMK config — project notes

Wireless KLOR keyboard (Saegewerk 38-key layout, nice!nano v2 clones), ZMK firmware,
split BT/USB with an SSD1306 OLED on the left/central half. Built via GitHub Actions
CI (`.github/workflows/build.yml` → `zmkfirmware/zmk`'s reusable `build-user-config.yml`),
no local `west`/Zephyr toolchain — every test cycle is push → CI build → flash real
hardware → observe.

## Current state (as of the `minimal-test` → `main` merge)

- Keymap: 3x5 + 3 thumb keys, QWERTY/Colemak-DH × Win/Mac (4 base layers) + Num/Nav/
  Sym/Admin/Func, homerow mods, combos. Synced from `wireless-corne-zmk-config`.
- Left half: full OLED status screen — BT/profile badge (blinking "BT..." while
  searching, "BT<n>" when connected), battery/charge badge, battery %, 4 live
  modifier badges, active layer name. Colors: black bg/white fg by default,
  inverts to white bg/black fg only when a mod is actually held (see Gotchas).
- Right half: **display is off** (`klor_right.conf` has no `CONFIG_ZMK_DISPLAY`).
  Right OLED was reported not lighting up on real hardware — untested whether
  that's hardware (loose connection/solder) or would just work if re-enabled;
  check with a multimeter before spending more time on it.
- Excluded from this build (commented out in `CMakeLists.txt`, straightforward to
  re-add now that the real root cause below is fixed): `klor_face_icon.c`,
  `klor_modifier_sync_central.c`/`_peripheral.c` (GATT sync forwarding right-hand
  mods to the peripheral's own display), `klor_display_power.c` (custom blanking —
  stock ZMK idle-blank is used instead, and that's fine as-is, no complaints on it).
  `icon_font.c` is compiled in but not used by any widget yet.
- `config/klor.keymap` (moved here from `config/boards/shields/klor/klor.keymap`,
  2026-08-04) is now the single source of truth for the keymap — this is
  intentional, not a repeat of the stale-duplicate incident described below.
  ZMK's `post_boards_shields.cmake` searches `ZMK_CONFIG` (`config/`) *before*
  the shield's own directory and stops at the first `<name>.keymap` match, so
  a top-level file is the standard override location, not a shadow-risk, as
  long as there's only ever one copy. It moved here specifically so
  https://nickcoutsos.github.io/keymap-editor/ can find it (it only looks in
  `config/`, not nested shield directories) — `config/klor.json` alongside it
  is the corresponding physical-layout file the editor needs to render the
  Saegewerk layout, replacing a stale generic-grid `klor.json` left over from
  the original repo template (`72904cb`, pre-dates this project's actual
  work) that didn't match this board's real position numbering or stagger at
  all.
- ZMK Studio and the `settings_reset` build were dropped for this branch's rebuild;
  re-add `studio-rpc-usb-uart` snippet + `CONFIG_ZMK_STUDIO` if Studio support comes
  back. `settings_reset` in `build.yaml` right now is a leftover diagnostic build —
  harmless to keep, or drop it if you don't need on-demand NVS wipes.

## How this branch came to exist

The keyboard broke in stages (dead key, garbled row, wrong thumb keys → traced to a
stale top-level `config/klor.keymap` shadowing the real shield keymap) and then,
once display work started, in a much nastier way: adding real widget code made
BT/USB stop working entirely, with no way to see why (no serial device enumerates
when this crashes — see Gotchas). After a long partial investigation on `main`
stalled out unresolved (see `c971ae0`/`9b2f75a`, 2026-07-30), `minimal-test` was
created to strip the config down to just a keymap and rebuild everything back up
one variable at a time, confirming after each addition. That bisection is what
found the two real root causes below — both had been live bugs since before
`minimal-test` even existed.

## Root causes found (the actual bugs, not the workarounds)

**1. Display work ran on the wrong work queue → BT/USB crash-loop.**
ZMK's `ZMK_DISPLAY_WORK_QUEUE` choice defaults to `ZMK_DISPLAY_WORK_QUEUE_SYSTEM`
(first option, no explicit `default` set upstream) — display rendering runs on
Zephyr's shared system workqueue, not a dedicated thread. Its stack is thin and
shared with other system work. A second styled label's worth of extra draw-call
stack usage (glyph decode + blit) was enough to overflow it, hard-faulting the
whole device before BT/USB finished initializing — read as "no BT, no keys, static
screen" with no crash-reset boundary visible from outside. Fixed in `klor_left.conf`:
```
CONFIG_ZMK_DISPLAY_WORK_QUEUE_DEDICATED=y
CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=4096
```

**2. Wrong Kconfig symbol for the LVGL memory pool → starved to 2KB, silent no-op.**
`CONFIG_LV_MEM_SIZE_KILOBYTES` (what earlier attempts set to "fix" memory issues)
**does not exist anywhere in `zmkfirmware/zmk`** — confirmed via `gh api
search/code`, zero matches. Zephyr's Kconfig silently drops unrecognized `CONFIG_`
lines in a `.conf` fragment; no error, no warning. The real symbol is
`LV_Z_MEM_POOL_SIZE` (`app/src/display/Kconfig`), which only gets a 4096-byte
default when `ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN` is selected — with
`STATUS_SCREEN_CUSTOM` (what this project uses) it silently fell through to
LVGL's own upstream default of **2048 bytes**, confirmed by reading the actual
Kconfig dump in a CI build log. This is what produced the maddening "one label
renders fine, add a second one and the whole display goes to static/snow, no
crash, BT/USB stay up" pattern across dozens of bisection commits — it wasn't
about which specific object, font, or container was added, it was about running
out of a 2KB heap. Fixed in `klor_left.conf`:
```
CONFIG_LV_Z_MEM_POOL_SIZE=65536
```
If display problems ever come back and look "impossible" (works with N objects,
breaks with N+1, no clear pattern) — check this Kconfig symbol first before
re-bisecting object-by-object.

## Gotchas / traps for next time

- **No serial console when things crash.** `CONFIG_ZMK_USB_LOGGING=y` plus a
  dedicated `log_uart0` CDC-ACM node (`klor_left.overlay`, separate from ZMK
  Studio's `studio-rpc-usb-uart` snippet — they need their own UART each, can't
  share one) usually gives `/dev/tty.usbmodem*`. But if the device is crash-
  looping, USB never finishes enumerating and no tty node appears at all — you
  are debugging blind. Confirm crash-loops are actually fixed (BT connects, keys
  register, reliably, across replugs) before trusting any serial capture.
- **Serial capture is flaky even when the device is healthy, for reasons that
  were never fully root-caused this session** — `screen` sessions reliably die
  the instant the USB device disconnects (so plugging AFTER starting the wait-
  loop, not before, matters), and even accounting for that, real capture
  attempts sometimes came back completely silent despite the firmware being
  provably fine (BT/keys working, binary byte-identical to a build that logged
  successfully). Don't sink hours into this like this session did — if 2-3
  capture attempts come back empty, fall back to visual-only bisection (change
  one thing, reflash, look at the panel) rather than continuing to fight the
  logging pipeline.
- **`inversion-on` (SSD1306 devicetree property, `klor_common.dtsi`) flips the
  *entire physical panel's* pixel polarity**, including every explicit
  `lv_color_black()`/`lv_color_white()` call anywhere in the codebase — not
  just the screen background. `klor_badge_create()`/`klor_badge_set_active()`
  (`klor_widgets_util.c`) and `klor_rule()` (`klor_central_widget.c`) have their
  black/white calls deliberately swapped to compensate; this was verified
  against real hardware, not derived from the color names, so don't "simplify"
  them back without retesting on the panel. If you add any *new* code that
  draws raw colors (not through `klor_badge_*`), it needs the same swap.
- **`lv_obj_align()` is a one-time position command, not a live constraint.**
  Aligning an empty `LV_SIZE_CONTENT` container to an edge (e.g. `TOP_RIGHT`)
  *before* adding its children anchors it at its zero-width size at call time;
  growing afterward as children are added pushes it in the wrong direction
  instead of growing from the anchored edge. Always create children first,
  align last, for any edge-anchored `LV_SIZE_CONTENT` row/container.
- **Flash the UF2 bootloader properly**: `cp firmware.uf2 /Volumes/NICENANO &&
  sync && while [ -d /Volumes/NICENANO ]; do sleep 0.2; done` — wait for the
  volume to actually disappear (bootloader auto-reboots into the new firmware
  once the write completes) before unplugging. A `cp` that returns quickly
  doesn't guarantee the write finished; macOS can buffer it.
- **CI build/module caching has been stale before** in this exact repo (traced
  once this session to `gh cache list`/`gh cache delete` fixing an "identical
  source, different binary" mystery). If a rebuild ever seems to ignore a real
  source change, verify the actual UF2 content (decode it, `strings` it, or
  just `md5` two builds you expect to differ) before assuming the *code* is
  wrong — don't take a green CI build's correctness for granted if the symptom
  looks cache-shaped.
- **`config/CMakeLists.txt` is only picked up because `zephyr/module.yml`
  registers `config/` as a Zephyr module** (`build: cmake:
  config/boards/shields/klor`). Without that, this file — and everything it
  wires in — is silently never included in the build. If a future refactor
  moves or restructures the shield directory, that module registration needs
  to move with it.
- **CI copies `ZMK_CONFIG` into an isolated temp dir for the actual build**,
  which means `CMakeLists.txt` can get processed from two different physical
  paths in the same build. The `GLOBAL PROPERTY KLOR_CMAKELISTS_INCLUDED` guard
  at the top of `config/boards/shields/klor/CMakeLists.txt` exists specifically
  for this — a plain `set()` variable wouldn't reliably share scope across both
  inclusion paths, the guard has to be a CMake global property.
