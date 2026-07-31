/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* minimal-test branch, step 4: bare minimum display -- just "TEST" on
 * screen. No widgets, no klor_central_widget.c, no klor_display_power.c
 * blanking logic, no color inversion. Bisecting from the confirmed-working
 * no-display baseline. */

#include <zephyr/kernel.h>
#include <lvgl.h>

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_label_set_text(label, "TEST");
    lv_obj_center(label);
    return screen;
}
