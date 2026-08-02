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
    /* Right-anchored LV_SIZE_CONTENT row -- re-aligned on every render, not
     * just at init, since bat_badge's text/font (BAT vs the bolt icon) and
     * pct_badge's digit count both change its width, and lv_obj_align() is
     * a one-time position command (see the alignment gotcha this project
     * keeps hitting). Without re-aligning, a width change shifts the row's
     * right edge past the panel edge instead of staying flush. */
    lv_obj_t *status_row;
    struct klor_badge bt_badge;
    struct klor_badge bat_badge;
    struct klor_badge pct_badge;
    struct klor_badge mod_badges[4];
    lv_obj_t *face_icon;
    struct klor_badge layer_name_badge;
};

int klor_central_widget_init(struct klor_central_widget *widget, lv_obj_t *parent);
lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget);

/* Shadow-tracked mods (display-only, not real HID state -- see
 * klor_central_widget.c's shadow-tracking section), 8-bit HID shape (bits
 * 0-3 left, 4-7 right). Used by klor_modifier_sync_central.c to forward
 * the right-hand nibble to the peripheral. */
uint8_t klor_central_widget_get_display_mods(void);
