/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

#include "klor_widgets_util.h"

struct klor_central_widget {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *bt_icon;
    struct klor_battery_bar battery;
    lv_obj_t *layer_label;
    lv_obj_t *os_icon;
    lv_obj_t *lock_num;
    lv_obj_t *lock_caps;
    lv_obj_t *lock_scroll;
};

int klor_central_widget_init(struct klor_central_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget);
