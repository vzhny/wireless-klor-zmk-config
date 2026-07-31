/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* minimal-test branch, step 6: real widget rendering restored
 * (klor_central_widget.c / klor_peripheral_widget.c). Still no
 * klor_display_power.c blanking (stock ZMK idle-blank for now,
 * intentionally) and no klor_modifier_sync_central.c GATT sync. */

#include <zephyr/kernel.h>
#include <lvgl.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
#include "widgets/klor_central_widget.h"
static struct klor_central_widget central_widget;
#else
#include "widgets/klor_peripheral_widget.h"
static struct klor_peripheral_widget peripheral_widget;
#endif

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    /* Black bg with white/lit elements on top -- badges, rule lines and
     * CONFIG_ZMK_DISPLAY_INVERT (klor_left.conf/klor_right.conf) all assume
     * this. A white screen bg here inverted that, washing everything out. */
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    klor_central_widget_init(&central_widget, screen);
    lv_obj_align(klor_central_widget_obj(&central_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#else
    klor_peripheral_widget_init(&peripheral_widget, screen);
    lv_obj_align(klor_peripheral_widget_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    return screen;
}
