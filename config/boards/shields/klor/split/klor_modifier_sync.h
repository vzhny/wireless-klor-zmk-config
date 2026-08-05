/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/bluetooth/uuid.h>

/*
 * Custom BLE GATT service: central writes R-modifier state to peripheral,
 * ported from wireless-corne-zmk-config's modifier_sync (same mechanism,
 * distinct UUIDs).
 *
 * Payload byte encoding:
 *   bits 0-3 = r_mods nibble (matches HID mod byte >> 4):
 *     bit 0 = RCtrl
 *     bit 1 = RShift
 *     bit 2 = RAlt
 *     bit 3 = RGUI
 *   bit 4 = Mac/Win glyph-order flag (mirrors klor_central_widget.c's
 *           mac_mode). The peripheral has no local keymap/layer state of
 *           its own to derive this from -- ZMK only resolves layers on the
 *           central half -- so it has to be forwarded here rather than read
 *           via zmk_keymap_layer_active() on the peripheral side.
 *   bit 5 = unused. Used to be a Qwerty/Colemak flag driving the
 *           peripheral's base_layer_badge; Qwerty's removed for now
 *           (Colemak-DH only, see readme.md), so that badge is just
 *           hardcoded to "COLEMAK" and this bit is never set or read.
 *   bit 6 = Bootloader-pending flag. One-way latch: once set, klor.keymap's
 *           bootloader_hold combo has passed its 1-second display-warning
 *           mark (see klor_central_widget.c), and both halves' screens
 *           switch to a centered "BOOTLOADER" label for good -- this never
 *           gets cleared back to 0 in software, only by an actual power
 *           cycle. Central sets it once via klor_modifier_sync_set_
 *           bootloader_pending() and never unsets it again.
 */

#define BT_UUID_KLOR_MOD_SVC_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000000ULL)

#define BT_UUID_KLOR_MOD_CHAR_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000001ULL)

/* Central-side only: pushes the current mod/layer state to the peripheral
 * right now, rather than waiting for the next layer event to do it.
 * klor_central_widget.c calls this whenever its shadow-tracked mods change
 * (see its shadow-tracking section), since that doesn't otherwise generate
 * any event klor_modifier_sync_central.c would normally see. */
void klor_modifier_sync_notify_mods_changed(void);

/* Central-side only: latches the bootloader-pending flag (bit 6 above) on
 * and immediately pushes it to the peripheral. One-way -- there is no
 * corresponding "clear" function, see the payload doc above. */
void klor_modifier_sync_set_bootloader_pending(void);
