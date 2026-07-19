/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* Recreates the QMK KLOR "master side" OLED screen (geist/default keymaps,
 * render_os_lock_status() + layer name) pixel-for-pixel using the same
 * glyphs from lib/glcdfont.c: layer name, OS mode icon (win/mac), the KLOR
 * face icon, and num/caps/scroll lock icons, separated by rule lines in the
 * same grouping/order as the original.
 *
 * QMK's dynamic-macro record/stop/play icon and the audio/haptic-enabled
 * icons are intentionally omitted -- ZMK has no equivalent runtime macro
 * recording feature, and this build has no audio or haptic driver.
 *
 * Battery and BT/output status (bottom row) are not part of the QMK
 * display -- that build is wired/USB-only. Added here using ZMK's own
 * built-in widgets, since this KLOR build is BLE.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/output_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/keymap.h>

#include "klor_central_widget.h"

LV_IMG_DECLARE(klor_face_icon);
LV_IMG_DECLARE(klor_os_mac_icon);
LV_IMG_DECLARE(klor_os_win_icon);
LV_IMG_DECLARE(klor_lock_num_icon);
LV_IMG_DECLARE(klor_lock_caps_icon);
LV_IMG_DECLARE(klor_lock_scroll_icon);

/* &tog'd base-layer indices from klor.keymap: 1 = Qwerty (Mac), 3 = Colemak-DH (Mac) */
#define KLOR_MAC_LAYER_A 1
#define KLOR_MAC_LAYER_B 3

/* Standard USB HID keyboard LED report bit order */
#define KLOR_LED_NUM_LOCK BIT(0)
#define KLOR_LED_CAPS_LOCK BIT(1)
#define KLOR_LED_SCROLL_LOCK BIT(2)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct klor_central_state {
    const char *layer_label;
    bool mac_mode;
    zmk_hid_indicators_t indicators;
};

static void klor_central_render(struct klor_central_state state) {
    struct klor_central_widget *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.layer_label != NULL && strlen(state.layer_label) > 0) {
            lv_label_set_text(widget->layer_label, state.layer_label);
        } else {
            lv_label_set_text(widget->layer_label, "");
        }

        lv_img_set_src(widget->os_icon, state.mac_mode ? &klor_os_mac_icon : &klor_os_win_icon);

        /* QMK shows a blank placeholder glyph in place of an inactive lock
         * icon, keeping its grid slot -- since these are absolute-positioned,
         * simply hiding/showing the icon in place has the same visual
         * effect. */
        if (state.indicators & KLOR_LED_NUM_LOCK) {
            lv_obj_clear_flag(widget->lock_num, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->lock_num, LV_OBJ_FLAG_HIDDEN);
        }

        if (state.indicators & KLOR_LED_CAPS_LOCK) {
            lv_obj_clear_flag(widget->lock_caps, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->lock_caps, LV_OBJ_FLAG_HIDDEN);
        }

        if (state.indicators & KLOR_LED_SCROLL_LOCK) {
            lv_obj_clear_flag(widget->lock_scroll, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(widget->lock_scroll, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static struct klor_central_state klor_central_get_state(const zmk_event_t *_eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();

    return (struct klor_central_state){
        .layer_label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
        .mac_mode = zmk_keymap_layer_active(KLOR_MAC_LAYER_A) ||
                    zmk_keymap_layer_active(KLOR_MAC_LAYER_B),
        .indicators = zmk_hid_indicators_get_current_profile(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_klor_central, struct klor_central_state,
                            klor_central_render, klor_central_get_state)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_layer_state_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_hid_indicators_changed)

static lv_obj_t *klor_rule(lv_obj_t *parent, lv_coord_t w, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, w, 1);
    lv_obj_set_style_bg_color(line, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
    lv_obj_align(line, LV_ALIGN_TOP_LEFT, x, y);
    return line;
}

int klor_central_widget_init(struct klor_central_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    /* Layer name -- row 0 */
    widget->layer_label = lv_label_create(widget->obj);
    lv_obj_align(widget->layer_label, LV_ALIGN_TOP_LEFT, 0, 0);

    /* Rule -- row 1 */
    klor_rule(widget->obj, 128, 0, 11);

    /* OS icon (left) + KLOR face icon (right) -- rows 2-3 */
    widget->os_icon = lv_img_create(widget->obj);
    lv_obj_align(widget->os_icon, LV_ALIGN_TOP_LEFT, 0, 15);

    lv_obj_t *face_icon = lv_img_create(widget->obj);
    lv_img_set_src(face_icon, &klor_face_icon);
    lv_obj_align(face_icon, LV_ALIGN_TOP_RIGHT, 0, 15);

    /* Rule -- row 4 */
    klor_rule(widget->obj, 128, 0, 33);

    /* Num/Caps/Scroll lock icons -- row 5 */
    widget->lock_num = lv_img_create(widget->obj);
    lv_img_set_src(widget->lock_num, &klor_lock_num_icon);
    lv_obj_align(widget->lock_num, LV_ALIGN_TOP_LEFT, 0, 37);

    widget->lock_caps = lv_img_create(widget->obj);
    lv_img_set_src(widget->lock_caps, &klor_lock_caps_icon);
    lv_obj_align(widget->lock_caps, LV_ALIGN_TOP_LEFT, 14, 37);

    widget->lock_scroll = lv_img_create(widget->obj);
    lv_img_set_src(widget->lock_scroll, &klor_lock_scroll_icon);
    lv_obj_align(widget->lock_scroll, LV_ALIGN_TOP_LEFT, 28, 37);

    /* Battery + output status -- bottom row (not present in QMK's wired-only
     * display; added since this build is BLE) */
    static struct zmk_widget_battery_status battery_widget;
    zmk_widget_battery_status_init(&battery_widget, widget->obj);
    lv_obj_align(zmk_widget_battery_status_obj(&battery_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);

    static struct zmk_widget_output_status output_widget;
    zmk_widget_output_status_init(&output_widget, widget->obj);
    lv_obj_align(zmk_widget_output_status_obj(&output_widget), LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    sys_slist_append(&widgets, &widget->node);

    widget_klor_central_init();

    return 0;
}

lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget) { return widget->obj; }
