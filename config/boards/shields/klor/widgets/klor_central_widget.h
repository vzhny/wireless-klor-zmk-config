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
    struct klor_badge bt_badge;
    struct klor_badge profile_badge;
    struct klor_badge bat_badge;
    struct klor_badge pct_badge;
    struct klor_badge mod_badges[4];
    lv_obj_t *face_icon;
    lv_obj_t *layer_label;
    struct klor_badge layer_badges[9];
};

int klor_central_widget_init(struct klor_central_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget);
