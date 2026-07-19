/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

/* Battery bar icon (22x10 shell + 2x4 nub) + percentage label, matching the
 * wireless-corne-zmk-config status-strip battery glyph. */
struct klor_battery_bar {
    lv_obj_t *fill;
    lv_obj_t *label;
};

void klor_battery_bar_create(struct klor_battery_bar *bar, lv_obj_t *parent, lv_coord_t x,
                             lv_coord_t y);
void klor_battery_bar_update(struct klor_battery_bar *bar, uint8_t level);
