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
    struct klor_badge link_badge;
    struct klor_badge bat_badge;
    struct klor_badge pct_badge;
    lv_obj_t *face_icon;
    struct klor_badge base_layer_badge;
    lv_obj_t *bootloader_label;
};

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget);

/* Called from klor_modifier_sync_peripheral.c's GATT write handler (BT RX
 * thread context -- this queues a redraw on the display work queue rather
 * than touching LVGL objects directly). One-way latch, see
 * klor_modifier_sync.h. */
void klor_peripheral_widget_set_bootloader_pending(void);
