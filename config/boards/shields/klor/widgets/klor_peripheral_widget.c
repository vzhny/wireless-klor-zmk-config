/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* Recreates the QMK KLOR "secondary side" OLED screen exactly: the full-screen
 * klor_face bitmap (geist/default keymaps, oled_task_kb()'s else branch).
 *
 * The top status strip (WiFi/link glyph + battery/%) is not part of the QMK
 * display -- that build is wired/USB-only. Added here in the same style as
 * wireless-corne-zmk-config's peripheral status strip (link glyph left,
 * battery+% right, reusing the WiFi glyph as a central-link indicator),
 * since this KLOR build is BLE and both halves are battery-powered. It's
 * drawn over a white strip so it stays legible against the face bitmap.
 */

#include <zephyr/kernel.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/battery.h>
#include <zmk/split/bluetooth/peripheral.h>

#include "klor_peripheral_widget.h"
#include "klor_widgets_util.h"

LV_IMG_DECLARE(klor_face_full);
LV_IMG_DECLARE(klor_wifi_icon);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct klor_peripheral_state {
    uint8_t battery_level;
    bool link_connected;
};

static void klor_peripheral_render(struct klor_peripheral_state state) {
    struct klor_peripheral_widget *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.link_connected) {
            lv_obj_clear_flag(widget->wifi_icon, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->wifi_icon, LV_OBJ_FLAG_HIDDEN);
        }

        klor_battery_bar_update(&widget->battery, state.battery_level);
    }
}

static struct klor_peripheral_state klor_peripheral_get_state(const zmk_event_t *_eh) {
    return (struct klor_peripheral_state){
        .battery_level = zmk_battery_state_of_charge(),
        .link_connected = zmk_split_bt_peripheral_is_connected(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_klor_peripheral, struct klor_peripheral_state,
                            klor_peripheral_render, klor_peripheral_get_state)
ZMK_SUBSCRIPTION(widget_klor_peripheral, zmk_battery_state_changed)
ZMK_SUBSCRIPTION(widget_klor_peripheral, zmk_split_peripheral_status_changed)

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    lv_obj_t *face = lv_img_create(widget->obj);
    lv_img_set_src(face, &klor_face_full);
    lv_obj_align(face, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Status strip background, so the strip stays legible over the face */
    lv_obj_t *strip = lv_obj_create(widget->obj);
    lv_obj_set_size(strip, 128, 16);
    lv_obj_set_style_radius(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(strip, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(strip, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(strip, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_align(strip, LV_ALIGN_TOP_LEFT, 0, 0);

    widget->wifi_icon = lv_img_create(widget->obj);
    lv_img_set_src(widget->wifi_icon, &klor_wifi_icon);
    lv_obj_align(widget->wifi_icon, LV_ALIGN_TOP_LEFT, 0, 1);

    klor_battery_bar_create(&widget->battery, widget->obj, 78, 3);

    sys_slist_append(&widgets, &widget->node);

    widget_klor_peripheral_init();

    return 0;
}

lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget) {
    return widget->obj;
}
