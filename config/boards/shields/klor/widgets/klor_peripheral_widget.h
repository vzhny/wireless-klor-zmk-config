/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#include "klor_widgets_util.h"

struct klor_peripheral_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *wifi_icon;
    struct klor_battery_bar battery;
};

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget);
