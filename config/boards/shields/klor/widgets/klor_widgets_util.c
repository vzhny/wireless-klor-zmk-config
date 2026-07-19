/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "klor_widgets_util.h"

#define KLOR_BATT_SHELL_W 22
#define KLOR_BATT_SHELL_H 10
#define KLOR_BATT_NUB_W 2
#define KLOR_BATT_NUB_H 4

static lv_obj_t *klor_static_rect(lv_obj_t *parent, lv_coord_t w, lv_coord_t h, lv_coord_t x,
                                  lv_coord_t y, bool filled) {
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_set_size(r, w, h);
    lv_obj_set_style_radius(r, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(r, 0, LV_PART_MAIN);
    if (filled) {
        lv_obj_set_style_bg_color(r, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(r, 0, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_opa(r, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_color(r, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_border_width(r, 1, LV_PART_MAIN);
    }
    lv_obj_align(r, LV_ALIGN_TOP_LEFT, x, y);
    return r;
}

void klor_battery_bar_create(struct klor_battery_bar *bar, lv_obj_t *parent, lv_coord_t x,
                             lv_coord_t y) {
    /* shell */
    klor_static_rect(parent, KLOR_BATT_SHELL_W, KLOR_BATT_SHELL_H, x, y, false);
    /* nub */
    klor_static_rect(parent, KLOR_BATT_NUB_W, KLOR_BATT_NUB_H, x + KLOR_BATT_SHELL_W,
                     y + (KLOR_BATT_SHELL_H - KLOR_BATT_NUB_H) / 2, true);
    /* fill -- width updated on every battery event */
    bar->fill = klor_static_rect(parent, 0, KLOR_BATT_SHELL_H - 2, x + 1, y + 1, true);

    bar->label = lv_label_create(parent);
    lv_obj_align(bar->label, LV_ALIGN_TOP_LEFT, x + KLOR_BATT_SHELL_W + KLOR_BATT_NUB_W + 4, y);
}

void klor_battery_bar_update(struct klor_battery_bar *bar, uint8_t level) {
    lv_coord_t fill_w = ((KLOR_BATT_SHELL_W - 2) * level) / 100;
    lv_obj_set_width(bar->fill, fill_w);

    char buf[6];
    snprintf(buf, sizeof(buf), "%d%%", level);
    lv_label_set_text(bar->label, buf);
}
