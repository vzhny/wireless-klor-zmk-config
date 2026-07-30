/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
#include "widgets/klor_central_widget.h"
static struct klor_central_widget central_widget;
#else
#include "widgets/klor_peripheral_widget.h"
static struct klor_peripheral_widget peripheral_widget;
#endif

lv_obj_t *zmk_display_status_screen(void) {
    LOG_DBG("klor_status_screen: zmk_display_status_screen called");
    lv_obj_t *screen = lv_obj_create(NULL);
    LOG_DBG("klor_status_screen: lv_obj_create(NULL) returned %p", (void *)screen);
    if (screen == NULL) {
        LOG_ERR("klor_status_screen: lv_obj_create(NULL) returned NULL, disp=%p",
                (void *)lv_disp_get_default());
        return NULL;
    }
    /* Black bg with white/lit elements on top -- badges, rule lines and
     * CONFIG_ZMK_DISPLAY_INVERT (klor_left.conf/klor_right.conf) all assume
     * this. A white screen bg here inverted that, washing everything out. */
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT)
    LOG_DBG("klor_status_screen: calling klor_central_widget_init");
    klor_central_widget_init(&central_widget, screen);
    lv_obj_align(klor_central_widget_obj(&central_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#else
    LOG_DBG("klor_status_screen: calling klor_peripheral_widget_init");
    klor_peripheral_widget_init(&peripheral_widget, screen);
    lv_obj_align(klor_peripheral_widget_obj(&peripheral_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    LOG_DBG("klor_status_screen: returning screen %p", (void *)screen);
    return screen;
}
