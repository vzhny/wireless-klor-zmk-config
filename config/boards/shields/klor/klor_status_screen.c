/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* minimal-test branch, step 7: real widget rendering restored, klor_face_icon.c
 * still excluded -- narrowing the BT/USB failure within klor_central_widget.c. */

#include <zephyr/kernel.h>
#include <lvgl.h>

#include "fonts/pixel_operator_mono.h"

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
#include "widgets/klor_central_widget.h"
static struct klor_central_widget central_widget;
#else
#include "widgets/klor_peripheral_widget.h"
static struct klor_peripheral_widget peripheral_widget;
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

    /* Visual-only bisection (no serial log needed): if this "TEST" label
     * shows up on the panel, zmk_display_status_screen() is definitely
     * being called/linked correctly and the bug is inside
     * klor_central_widget_init()'s execution somehow preventing a normal
     * return. If the panel stays static even with this, the function
     * itself isn't being reached -- confirms the weak-symbol-override
     * theory instead. */
    lv_obj_t *canary = lv_label_create(screen);
    lv_obj_set_style_text_color(canary, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(canary, &pixel_operator_mono, LV_PART_MAIN);
    lv_label_set_text(canary, "TEST");
    lv_obj_align(canary, LV_ALIGN_BOTTOM_MID, 0, 0);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    klor_central_widget_init(&central_widget, screen);
    lv_obj_align(klor_central_widget_obj(&central_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#else
    klor_peripheral_widget_init(&peripheral_widget, screen);
    lv_obj_align(klor_peripheral_widget_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}
