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
    struct klor_badge mod_badges[4];
};

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget);

/* Called from klor_modifier_sync_peripheral.c's GATT write handler (BT RX
 * thread context, not the display thread -- this queues a redraw on the
 * display work queue rather than touching LVGL objects directly).
 *
 * payload bits 0-3: r_mods nibble. bit 4: Mac/Win glyph-order flag, forwarded
 * by central since this half has no local keymap/layer state to derive it
 * from (see klor_modifier_sync.h). */
void klor_peripheral_widget_update_mods(uint8_t payload);
