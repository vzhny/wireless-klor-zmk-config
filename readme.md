<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/docs/images/klor-font-logo-dark.svg">
  <source media="(prefers-color-scheme: light)" srcset="/docs/images/klor-font-logo-bright.svg">
  <img alt="KLOR logo font" src="/docs/images/klor-font-logo-bright.svg">
</picture>

## THIS BUILD

This config runs the **Saegewerk** break-off layout: 38 keys total, 3 rows of
5 per hand plus 2 inner media keys and a 3-key thumb cluster per hand. It's
the same PCB matrix as the other three KLOR layouts (Polydactyl, Konrad,
Yubitsume) — Saegewerk just leaves the outer pinky column unpopulated on rows
1–2 and drops the two innermost thumb keys that Polydactyl has.

Hardware: two nice!nano v2 clones, BLE split, USB fallback, one SSD1306
128×64 OLED per half.

### Features

- **2 base layers**: Colemak-DH only for the time being (Qwerty removed —
  see `klor.keymap`'s `colemak_win_layer` comment), Windows and macOS
  variants (Ctrl/GUI swapped on the homerow only). Boots in Colemak-DH (Win)
  always — the Mac toggle is runtime-only (`&tog`, not written to NVS) and
  resets on power cycle.
- **Homerow mods** (`hml`/`hmr`, balanced flavor, 200ms tapping term, 150ms
  require-prior-idle) on both hands' home row, tuned against accidental
  activation while typing normally.
- **Thumb dual-role keys** (`thm`): tap for Space/Backspace, hold for
  Shift — same balanced flavor as the homerow mods.
- **Layer-tap** on the outer pinky column: hold Z for Nav, hold `/` for Sym.
- **Conditional Admin layer**: holding Num + Func together (no dedicated key)
  activates BT profile switching and the default-layer/Mac `&to`/`&tog`
  toggles.
- **Bootloader shortcut**: a separate Num+Nav chord arms a hold-to-enter
  Bootloader layer, letting each half reboot into UF2 mode independently
  from the keyboard alone, no physical reset button — see
  [BOOTLOADER SHORTCUT](#bootloader-shortcut) below.
- **Combos**: Tab, Enter, Esc, `'`, Cut/Copy/Paste/Undo (with separate
  Win/Mac keybind variants that key off which base layer is active), and
  Caps Word (both thumb shifts together).
- **Per-half OLED status screen**: BT/profile state, battery, live modifier
  badges, and active layer name on the left; link/battery status and a
  mirrored modifier row on the right (see mockups below).

### Layers

Layer order and names match `klor.keymap` / the left OLED's layer-name badge
exactly. `·` marks `trans` (falls through to whichever base layer 0–1 is
currently active); `KEY-MOD` marks a homerow/thumb key that's tap-for-letter,
hold-for-modifier. Win layer uses `⌃` Ctrl / `⊞` Win (Gui) / `Alt` / `⇧`
Shift; Mac layer uses `⌃` Ctrl / `⌘` Cmd (Gui) / `⌥` Opt (Alt) / `⇧` Shift —
Windows has no well-supported single-glyph Unicode symbol for Alt, so that
one falls back to its short name. Every cell in every layer shares one fixed
width (set by the widest cell anywhere in the doc) so all 9 diagrams line up
identically. Left hand's box on the left, right hand's on the right (aligned
to the same column on every row, mirroring the physical gap between the two
halves); thumb clusters sit close together below, like the real thumb keys
do.

Both base layers (0–1) are fully spelled out (no `trans` fallthrough between
them) — see `klor.keymap`. Qwerty removed for now, Colemak-DH only; see the
Features section above.

The row-2 inner media keys (positions 25/26) aren't shown here — see the
encoder note below the diagrams.

```
[0] Colemak-DH (Win) -- default
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   Q    |   W    |   F    |   P    |   B    |          |   J    |   L    |   U    |   Y    |   ;    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|  A-⌃   |  R-⌘   |  S-⌥   |   T    |   G    |          |   M    |   N    |  E-⌥   |  I-⌘   |  O-⌃   |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
| Z-nav  |   X    |   C    |   D    |   V    |          |   K    |   H    |   ,    |   .    | /-sym  |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |  BSPC  |  Num   | SPC-⇧  |                            | BSPC-⇧ |  Func  |  RET   |
         +--------+--------+--------+                            +--------+--------+--------+

[1] Colemak-DH (Mac)
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   Q    |   W    |   F    |   P    |   B    |          |   J    |   L    |   U    |   Y    |   ;    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|  A-⌘   |  R-⌃   |  S-⌥   |   T    |   G    |          |   M    |   N    |  E-⌥   |  I-⌃   |  O-⌘   |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
| Z-nav  |   X    |   C    |   D    |   V    |          |   K    |   H    |   ,    |   .    | /-sym  |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |  BSPC  |  Num   | SPC-⇧  |                            | BSPC-⇧ |  Func  |  RET   |
         +--------+--------+--------+                            +--------+--------+--------+

[2] Numbers
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   1    |   2    |   3    |   4    |   5    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   6    |   7    |   8    |   9    |   0    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[3] Navigation
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |  HOME  |  PGDN  |  PGUP  |  END   |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |  VOL-  |  VOL+  |          |  LEFT  |  DOWN  |   UP   | RIGHT  |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |  BSPC  |  DEL   |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[4] Symbols
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   -    |   =    |   (    |   )    |   `    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   &    |   |    |   {    |   }    |   ~    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   *    |   /    |   [    |   ]    |   \    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[5] Function Keys
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   F1   |   F2   |   F3   |   F4   |   F5   |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   F6   |   F7   |   F8   |   F9   |  F10   |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|  F11   |  F12   |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[6] Admin (Num+Func chord)
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|  BT0   |  BT1   |  BT2   |  BT3   | BTCLR  |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
| D-Win0 | C-Mac1 |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[7] Arm (Num+Nav chord)
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    | V-arm  |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

[8] Bootloader (hold V to keep open)
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|  BOOT  |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |  BOOT  |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+
|   ·    |   ·    |   ·    |   ·    |   ·    |          |   ·    |   ·    |   ·    |   ·    |   ·    |
+--------+--------+--------+--------+--------+          +--------+--------+--------+--------+--------+

         +--------+--------+--------+                            +--------+--------+--------+
         |   ·    |   ·    |   ·    |                            |   ·    |   ·    |   ·    |
         +--------+--------+--------+                            +--------+--------+--------+

```

`D-Win0`/`C-Mac1` on the Admin layer are `&to 0`/`&tog 1` respectively —
`D-Win0` jumps straight back to Colemak-DH (Win), deactivating the Mac
toggle if it's on, so you always have a way home without a power cycle.
`V-arm` and both `BOOT` cells are the
Bootloader shortcut — see [BOOTLOADER SHORTCUT](#bootloader-shortcut) below
for how it works and how to use it; all three (`klor.keymap`'s
`admin_layer`/`arm_layer`/`bootloader_layer` comments) have the implementation
detail.

Each half also has a physical rotary encoder (`left_encoder`/`right_encoder`,
`klor.dtsi`) — separate hardware from the row-2 inner keys above. Its
rotation is bound to Volume Up/Down (`sensor-bindings`) on every layer; a
press/tap binding for Play/Pause isn't implemented.

### Display mockups

Both halves share one 128×64 monochrome SSD1306, physically inverted
(`inversion-on` in the devicetree) so black-bg/white-fg is the _idle_ look —
badges shown here as `▓TEXT▓` are the _active/lit_ state (flips to
white-bg/black-fg on the real panel); plain `[TEXT]` badges are idle.

**Left half (central)** — BT/profile + battery status, live modifier badges,
active layer name, KLOR face icon:

```
┌────────────────────────────────────────┐
│ [BT0]                    [BAT]  [ 87%] │
│ ---------------------------------------│
│ [SFT][CTL][GUI][ALT]                   │
│ ---------------------------------------│
│ Base                              ◠◠   │
└────────────────────────────────────────┘
```

Disconnected/pairing shows a blinking `[BT.]` → `[BT..]` → `[BT...]`; on USB
power the battery badge reads a bolt icon in place of `[BAT]`; holding e.g.
left Ctrl lights that badge: `▓CTL▓`; the layer badge tracks whatever's
active (`Num`, `Nav`, `Sym`, `Admin`, `Func`); Mac layers swap the mod-badge
order to `[SFT][CMD][CTL][OPT]`.

**Right half (peripheral)** — link + battery status, mirrored modifier row,
KLOR face icon, active base-layer name. The modifier row and base-layer name
are live — forwarded from the left half over a custom BLE GATT
characteristic on every keycode/layer change, not static:

```
┌────────────────────────────────────────┐
│ [UNLINKED]                [BAT] [ 87%] │
│ ---------------------------------------│
│                [ALT][GUI][CTL][SFT]    │
│ ---------------------------------------│
│ ◠◠                           [COLEMAK] │
└────────────────────────────────────────┘
```

Shows `[LINKED]` once the peripheral connects to the central half. The
base-layer badge always reads `[COLEMAK]` now (Qwerty removed, see
Features above) — it's no longer driven by a synced flag. The modifier row
still relabels to `[OPT][CTL][CMD][SFT]` when the left half is on the Mac
variant.

**Bootloader alert state** — both halves, once the
[Bootloader shortcut](#bootloader-shortcut)'s 1-second mark passes: row 3's
layer name/face icon (left) or face icon/base-layer badge (right) are
replaced by one centered, full-width label. Status row and modifier row
above it keep updating normally — only row 3 changes:

```
┌────────────────────────────────────────┐
│ [BT0]                    [BAT]  [ 87%] │
│ ---------------------------------------│
│ [SFT][CTL][GUI][ALT]                   │
│ ---------------------------------------│
│              BOOTLOADER                │
└────────────────────────────────────────┘
```

```
┌────────────────────────────────────────┐
│ [LINKED]                 [BAT]  [ 87%] │
│ ---------------------------------------│
│                [ALT][GUI][CTL][SFT]    │
│ ---------------------------------------│
│              BOOTLOADER                │
└────────────────────────────────────────┘
```

This is a one-way latch, not a live status — once either screen shows this,
software never reverts it, even if the actual reboot never fires (e.g. you
release before the 3-second mark, or the peripheral was unreachable). The
only way back to normal is power-cycling that half. See
[BOOTLOADER SHORTCUT](#bootloader-shortcut) for why.

## HOW TO USE

- fork this repo
- `git clone` your repo, to create a local copy on your PC (you can use the [command line](https://www.atlassian.com/git/tutorials) or [github desktop](https://desktop.github.com/))
- adjust the klor.keymap file (find all the keycodes on [the zmk docs pages](https://zmk.dev/docs/codes/))
- `git push` your repo to your fork
- on the GitHub page of your fork navigate to "Actions"
- scroll down and unzip the `firmware.zip` archive that contains the latest firmware
- connect the left half of the KLOR to your PC, press reset twice (or use the
  [keyboard-only shortcut](#bootloader-shortcut) below instead)
- the keyboard should now appear as a mass storage device
- drag'n'drop the `klor_left-nice_nano_v2-zmk.uf2` file from the archive onto the storage device
- repeat this process with the right half and the `klor_right-nice_nano_v2-zmk.uf2` file.

## BOOTLOADER SHORTCUT

Both halves can be dropped into UF2 bootloader mode from the keyboard alone
— no reaching for the tiny physical reset button on either board. This only
works while both halves are BLE-connected to each other (they don't need
TRRS/wired); it's meant to be triggered right before you flash, with both
halves still paired, not after separating them.

**Gesture:**

1. Hold the Num thumb key + `Z` briefly, then press and hold `V` — this
   "arms" `V` (see the `[7] Arm` layer above). Release the Num thumb key and
   `Z`; keep holding `V`. `V` alone now keeps the Bootloader layer open for
   as long as it stays held (ZMK resolves `V`'s own binding once, at its
   press, from whichever layers were active at that instant — releasing
   Num/Nav afterward doesn't undo that).
2. While still holding `V`, hold `;` for **3 seconds**. One second in, both
   screens latch to a centered `BOOTLOADER` label (see the mockups above) —
   that's your warning it's about to fire, not confirmation it already has.
   At 3 seconds, the **right/peripheral** half reboots into bootloader mode.
3. Release `;` (keep holding `V`). Once you've confirmed the right half is
   actually in bootloader mode (e.g. it shows up as a `NICENANO` drive, or
   you can otherwise tell it's no longer running as a keyboard), hold `Q`
   for 3 seconds. The **left/central** half reboots into bootloader mode.
4. Flash each half normally (see HOW TO USE above) — order between the two
   doesn't matter once both are actually in bootloader mode.

**Do the right half first, always.** Once the left (central) half reboots,
it stops running ZMK entirely — no keymap, no BLE central role — so the
right half has nothing left to resolve `;` through, and holding it
afterward does nothing. There's no way to recover from doing this out of
order except the physical reset button on the right half.

**Never hold `;` and `Q` at the same time.** If both 3-second holds
complete together, the left half's own reboot can cut its BLE radio before
the "reboot yourself" message to the right half has actually finished
sending over the air — leaving the right half un-rebooted with no
indication why. Do them strictly one at a time, right fully first.

**A quick tap of `V`, `;`, or `Q` does nothing** — `;`/`Q` both need the
full 3-second hold before anything happens (short of that, they're
`&none`), and letting go of `V` early just drops the Bootloader layer with
no side effects. The one exception: once either screen's 1-second
`BOOTLOADER` warning has shown, it's permanent for that half regardless of
what happens next (see the Display mockups section above) — only a power
cycle clears it, even if the reboot itself never actually fired.

This exists because ZMK's `&bootloader` behavior has
`locality = BEHAVIOR_LOCALITY_EVENT_SOURCE`: it reboots whichever physical
half a keypress actually originated from, not always the central — see
`klor.keymap`'s `bootloader_layer` comment for the full mechanism (and why
it has to be a direct keymap binding, not a combo).

## KNOWN ISSUES

- The encoder on the secondary side doesn't work yet. This is a limitation of ZMK.
