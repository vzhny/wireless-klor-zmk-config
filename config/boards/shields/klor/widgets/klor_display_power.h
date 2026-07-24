/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>

/* OLED "kill switch" -- see klor_display_power.c for the full rationale.
 * Compiled on both roles; each half only calls the entry points relevant to
 * its own role. */

/* Central only: this half's own BT-host connection state. */
void klor_display_power_bt_state(bool connected);

/* Central only: whether the peripheral (right) half currently has a live
 * BLE link to this half. ZMK exposes no public event for this on the
 * central role, so klor_modifier_sync_central.c's own connection callbacks
 * feed it in directly. */
void klor_display_power_peripheral_link_state(bool connected);

/* Peripheral only: whether this half currently has a live BLE link to the
 * central (left) half. */
void klor_display_power_link_state(bool connected);
