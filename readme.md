<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/docs/images/klor-font-logo-dark.svg">
  <source media="(prefers-color-scheme: light)" srcset="/docs/images/klor-font-logo-bright.svg">
  <img alt="KLOR logo font" src="/docs/images/klor-font-logo-bright.svg">
</picture>

# ZMK CONFIG FOR THE KLOR SPLIT KEYBOARD

[Here](https://github.com/GEIGEIGEIST/qmk-config-klor) you can find the QMK config for the KLOR.\
[Here](https://github.com/GEIGEIGEIST/klor) you can find the hardware files and build guide.

KLOR is a 36-42 key column-staggered split keyboard. It supports a per key RGB matrix, encoders, OLED displays, a Pixart Paw3204 trackball and four different layouts, through brake off parts.

![KLOR layouts](/docs/images/klor-layouts.svg)

Polydactyl is the default layout. If you choose one of the other layouts you can use the matching template in the default keymap.


## THIS BUILD

This config runs the **Saegewerk** break-off layout: 38 keys total, 3 rows of
5 per hand plus 2 inner media keys and a 3-key thumb cluster per hand. It's
the same PCB matrix as the other three KLOR layouts (Polydactyl, Konrad,
Yubitsume) — Saegewerk just leaves the outer pinky column unpopulated on rows
1–2 and drops the two innermost thumb keys that Polydactyl has.

Hardware: two nice!nano v2 clones, BLE split, USB fallback, one SSD1306
128×64 OLED per half.

### Features

- **4 base layers**: Qwerty and Colemak-DH, each with a Windows and a macOS
  variant (Ctrl/GUI swapped on the homerow only). Boots in Qwerty (Win)
  always — the Mac/Colemak toggles are runtime-only (`&tog`, not written to
  NVS) and reset on power cycle.
- **Homerow mods** (`hml`/`hmr`, balanced flavor, 200ms tapping term, 150ms
  require-prior-idle) on both hands' home row, tuned against accidental
  activation while typing normally.
- **Thumb dual-role keys** (`thm`): tap for Space/Backspace, hold for
  Shift — same balanced flavor as the homerow mods.
- **Layer-tap** on the outer pinky column: hold Z for Nav, hold `/` for Sym.
- **Conditional Admin layer**: holding Num + Func together (no dedicated key)
  activates BT profile switching, reset/bootloader, and the Mac/Colemak
  toggles.
- **Combos**: Tab, Enter, Esc, `'`, Cut/Copy/Paste/Undo (with separate
  Win/Mac keybind variants that key off which base layer is active), and
  Caps Word (both thumb shifts together).
- **Per-half OLED status screen**: BT/profile state, battery, live modifier
  badges, and active layer name on the left; link/battery status and a
  mirrored modifier row on the right (see mockups below).

### Layers

Layer order and names match `klor.keymap` / the left OLED's layer-name badge
exactly. `trans` falls through to whichever base layer (0–3) is currently
active; `⌐MOD` marks a homerow/thumb key that's tap-for-letter, hold-for-modifier.

```
[0] Qwerty (Win)                          [2] Colemak-DH (Win)
 Q     W     E     D     T     Y  U  I  O  P     Q     W     F     P     B     J  L  U  Y  ;
 A⌐⌃   S⌐⌘   D⌐⌥   F     G     H  J  K⌐⌥ L⌐⌘ ;⌐⌃   A⌐⌃   R⌐⌘   S⌐⌥   T     G     M  N  E⌐⌥ I⌐⌘ O⌐⌃
 Z⌐nav X     C     V     B  ⏸  ▶  N  M  ,  .  /⌐sym Z⌐nav X     C     D     V  ⏸  ▶  K  H  ,  .  /⌐sym
       BSPC  Num   SPC⌐⇧ BSPC⌐⇧ Func RET             BSPC  Num   SPC⌐⇧ BSPC⌐⇧ Func RET

[1] Qwerty (Mac) -- overlay on [0]        [3] Colemak-DH (Mac) -- full layer
 ·     ·     ·     ·     ·     ·  ·  ·  ·  ·     Q     W     F     P     B     J  L  U  Y  ;
 A⌐⌘   S⌐⌃   ·     ·     ·     ·  ·  ·  L⌐⌃ ;⌐⌘   A⌐⌘   R⌐⌃   S⌐⌥   T     G     M  N  E⌐⌥ I⌐⌃ O⌐⌘
 ·     ·     ·     ·     ·  ·  ·  ·  ·  ·  ·  ·     Z⌐nav X     C     D     V  ·  ·  K  H  ,  .  /⌐sym
       ·     ·     ·     ·     ·  ·                 BSPC  Num   SPC⌐⇧ BSPC⌐⇧ Func RET
 (· = trans, i.e. same key as layer 0)

[4] Numbers                               [5] Navigation
 1  2  3  4  5     ·  ·  ·  ·  ·           ·  ·  ·     ·     ·     HOME PGDN PGUP END  ·
 6  7  8  9  0     ·  ·  ·  ·  ·           ·  ·  ·     VOL-  VOL+  LEFT DOWN UP   RIGHT ·
 ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·  ·        ·  BSPC DEL ·     ·  ·  ·    ·    ·    ·     ·  ·
    ·     ·     ·     ·  ·                    ·    ·   ·     ·  ·

[6] Symbols                               [7] Admin (Num+Func chord)
 -  =  (  )  `      ·  ·  ·  ·  ·          BT0 BT1 BT2 BT3 BTCLR  RESET BOOT ·  ·  ·
 &  |  {  }  ~      ·  ·  ·  ·  ·          Mac1 Colemak2 Mac3-Colemak ·  ·   ·    ·  ·  ·  ·
 *  /  [  ]  \      ·  ·  ·  ·  ·  ·  ·    ·   ·   ·   ·   ·  ·  ·  ·   ·   ·  ·  ·
    ·     ·     ·     ·  ·                     ·   ·   ·   ·   ·  ·

[8] Function Keys
 F1  F2  F3  F4  F5     ·  ·  ·  ·  ·
 F6  F7  F8  F9  F10    ·  ·  ·  ·  ·
 F11 F12 ·   ·   ·   ·  ·  ·  ·  ·  ·  ·
     ·   ·   ·   ·  ·
```

### Display mockups

Both halves share one 128×64 monochrome SSD1306, physically inverted
(`inversion-on` in the devicetree) so black-bg/white-fg is the *idle* look —
badges shown here as `▓TEXT▓` are the *active/lit* state (flips to
white-bg/black-fg on the real panel); plain `[TEXT]` badges are idle.

**Left half (central)** — BT/profile + battery status, live modifier badges,
active layer name:

```
┌────────────────────────────────────────┐
│ [BT0]                    [BAT]  [ 87%] │
│ ---------------------------------------│
│ [SFT][CTL][GUI][ALT]                   │
│ ---------------------------------------│
│ Base                                   │
└────────────────────────────────────────┘
```

Disconnected/pairing shows a blinking `[BT.]` → `[BT..]` → `[BT...]`; on USB
power the battery badge reads `[CHG]`; holding e.g. left Ctrl lights that
badge: `▓CTL▓`; the layer badge tracks whatever's active (`Num`, `Nav`,
`Sym`, `Admin`, `Func`).

**Right half (peripheral)** — link + battery status, KLOR face icon, mirrored
modifier row (currently static — live sync from the left half isn't wired up
yet, see `CLAUDE.md`):

```
┌────────────────────────────────────────┐
│ [LINKED]                 [BAT]  [ 87%] │
│ ---------------------------------------│
│  ◠◠                [ALT][GUI][CTL][SFT]│
│ ---------------------------------------│
└────────────────────────────────────────┘
```


## HOW TO USE

- fork this repo
- `git clone` your repo, to create a local copy on your PC (you can use the [command line](https://www.atlassian.com/git/tutorials) or [github desktop](https://desktop.github.com/))
- adjust the klor.keymap file (find all the keycodes on [the zmk docs pages](https://zmk.dev/docs/codes/))
- `git push` your repo to your fork
- on the GitHub page of your fork navigate to "Actions"
- scroll down and unzip the `firmware.zip` archive that contains the latest firmware
- connect the left half of the KLOR to your PC, press reset twice
- the keyboard should now appear as a mass storage device
- drag'n'drop the `klor_left-nice_nano_v2-zmk.uf2` file from the archive onto the storage device
- repeat this process with the right half and the `klor_right-nice_nano_v2-zmk.uf2` file.


## KNOWN ISSUES

- The encoder on the secondary side doesn't work yet. This is a limitation of ZMK.
- Need to add the code for the Pixart Paw3204 trackball.