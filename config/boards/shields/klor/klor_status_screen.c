/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* minimal-test branch, step 5: "TEST" label plus one badge in a row, to
 * exercise klor_widgets_util.c's flex-row/badge helpers. Still no
 * klor_central_widget.c, no klor_display_power.c blanking, no GATT sync. */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include "widgets/klor_widgets_util.h"

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_t *label = lv_label_create(screen);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_label_set_text(label, "TEST");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *row =
        klor_badge_row_create(screen, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, 0);

    static struct klor_badge badge;
    klor_badge_create(&badge, row, "BADGE");

    return screen;
}
