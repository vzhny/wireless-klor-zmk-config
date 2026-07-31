/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* minimal-test branch, step 6b: back to step 5's screen (just "TEST" +
 * one badge, no klor_central_widget.c). Testing whether the keymap-sync
 * changes alone are enough to reproduce the BT/USB failure. */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include "fonts/pixel_operator_mono_large.h"
#include "widgets/klor_widgets_util.h"

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, &pixel_operator_mono_large, LV_PART_MAIN);
    lv_label_set_text(label, "TEST");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *row =
        klor_badge_row_create(screen, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);

    static struct klor_badge badge;
    klor_badge_create(&badge, row, "BADGE");

    return screen;
}
