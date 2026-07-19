/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include <zephyr/kernel.h>

struct klor_peripheral_widget {
    lv_obj_t *obj;
};

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget);
