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
 * r_mods byte encoding (matches HID mod byte >> 4):
 *   bit 0 = RCtrl
 *   bit 1 = RShift
 *   bit 2 = RAlt
 *   bit 3 = RGUI
 */

#define BT_UUID_KLOR_MOD_SVC_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000000ULL)

#define BT_UUID_KLOR_MOD_CHAR_VAL \
    BT_UUID_128_ENCODE(0x4F2A8568, 0xC321, 0x4D23, 0x92CD, 0x000000000001ULL)
