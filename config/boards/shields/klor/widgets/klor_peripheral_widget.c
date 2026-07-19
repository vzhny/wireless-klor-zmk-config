/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* Recreates the QMK KLOR "secondary side" OLED screen exactly: the full-screen
 * klor_face bitmap (geist/default keymaps, oled_task_kb()'s else branch),
 * with nothing else drawn over it in the original.
 *
 * Battery status is not part of the QMK display -- that build is wired/USB-only
 * and has no battery. Added here in a corner using ZMK's own built-in battery
 * widget, since this KLOR build is BLE and both halves are battery-powered.
 */

#include <zephyr/kernel.h>

#include <zmk/display/widgets/battery_status.h>

#include "klor_peripheral_widget.h"

LV_IMG_DECLARE(klor_face_full);

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    lv_obj_t *face = lv_img_create(widget->obj);
    lv_img_set_src(face, &klor_face_full);
    lv_obj_align(face, LV_ALIGN_TOP_LEFT, 0, 0);

    static struct zmk_widget_battery_status battery_widget;
    zmk_widget_battery_status_init(&battery_widget, widget->obj);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    return 0;
}

lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget) {
    return widget->obj;
}
