/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>

/*
 * Custom BLE GATT service: central writes one byte to the peripheral, once,
 * when the bootloader warning latches. The peripheral has no keymap/layer
 * state of its own (ZMK resolves those on the central only), so this is how
 * its screen learns to show "BOOTLOADER" too.
 *
 * It used to carry the right-hand modifier nibble and a Mac/Win flag on
 * every mod change; the mod display was removed (see CLAUDE.md), and only
 * this bit is left. Bit 6 keeps its old position on purpose: a half still
 * running the old firmware writes mod bits in 0-4, which a new peripheral
 * must not read as "bootloader pending".
 *
 * One-way latch: never cleared in software, only by a power cycle.
 */

#define KLOR_SYNC_BOOTLOADER_PENDING BIT(6)

#define BT_UUID_KLOR_MOD_SVC_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000000ULL)

#define BT_UUID_KLOR_MOD_CHAR_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000001ULL)

/* Central-side only: writes the bootloader-pending flag to the peripheral.
 * One-way -- there is no corresponding "clear" function. */
void klor_modifier_sync_set_bootloader_pending(void);
