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
    struct klor_badge bat_badge;
    struct klor_badge pct_badge;
    lv_obj_t *face_icon;
    struct klor_badge layer_name_badge;
    lv_obj_t *bootloader_label;
};

int klor_central_widget_init(struct klor_central_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget);

/* One-way latch: once called, the left screen's row 3 permanently switches
 * to a centered "BOOTLOADER" label (see klor_central_widget.c's bootloader
 * warn-timer section). There is no corresponding "clear" function. */
void klor_central_widget_set_bootloader_pending(void);
