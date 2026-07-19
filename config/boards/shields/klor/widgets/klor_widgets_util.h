/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

/* A small auto-sized text badge: black background / white text normally,
 * inverts to white background / black text when marked active. Used for
 * every status indicator on both screens (BT/LINKED, BAT/CHG, battery %,
 * modifiers, layer numbers) so they all share one visual language. */
struct klor_badge {
    lv_obj_t *box;
    lv_obj_t *label;
};

/* row_align: LV_FLEX_ALIGN_START (left-packed) or LV_FLEX_ALIGN_END (right-packed) */
lv_obj_t *klor_badge_row_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                lv_flex_align_t row_align);

void klor_badge_create(struct klor_badge *badge, lv_obj_t *parent, const char *text);
void klor_badge_set_text(struct klor_badge *badge, const char *text);
void klor_badge_set_active(struct klor_badge *badge, bool active);
