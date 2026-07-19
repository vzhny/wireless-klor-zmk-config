/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* KLOR "secondary side" (right/peripheral) OLED screen.
 *
 * Status strip (LINKED/BAT/%) mirrors the central screen's BT/BAT/% strip.
 * The modifier row mirrors wireless-corne-zmk-config's peripheral screen:
 * right-hand modifier state doesn't exist locally on this half (ZMK only
 * resolves keycodes/mods on the central half), so it's received over a
 * custom BLE GATT characteristic the central half writes to on every
 * keycode event (see split/klor_modifier_sync_*.c, ported from corne's
 * modifier_sync). That GATT write lands on the BT RX thread, not the
 * display thread, so updates go through the display work queue rather
 * than touching LVGL objects directly -- this is the same mechanism
 * ZMK_DISPLAY_WIDGET_LISTENER uses internally for ordinary ZMK events.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/split/bluetooth/peripheral.h>

#include "klor_peripheral_widget.h"
#include "klor_widgets_util.h"

LV_IMG_DECLARE(klor_face_icon);

/* &tog'd base-layer indices from klor.keymap: 1 = Qwerty (Mac), 3 = Colemak-DH (Mac) */
#define KLOR_MAC_LAYER_A 1
#define KLOR_MAC_LAYER_B 3

/* r_mods nibble bit layout, matches klor_modifier_sync.h */
#define R_MOD_RCTRL BIT(0)
#define R_MOD_RSHIFT BIT(1)
#define R_MOD_RALT BIT(2)
#define R_MOD_RGUI BIT(3)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct klor_peripheral_state {
    uint8_t battery_level;
    bool charging;
    bool link_connected;
    bool mac_mode;
    uint8_t r_mods;
};

static struct klor_peripheral_state widget_state;

/* Right-side modifier slot order, mirrored (right-to-left) from
 * klor_central_widget.c's mod_slot(): Win = ALT,GUI,CTL,SFT / Mac = OPT,CTL,CMD,SFT */
static void mod_slot(bool mac_mode, int slot, const char **text, uint8_t *bit) {
    static const char *const win_text[4] = {"ALT", "GUI", "CTL", "SFT"};
    static const uint8_t win_bit[4] = {R_MOD_RALT, R_MOD_RGUI, R_MOD_RCTRL, R_MOD_RSHIFT};
    static const char *const mac_text[4] = {"OPT", "CTL", "CMD", "SFT"};
    static const uint8_t mac_bit[4] = {R_MOD_RALT, R_MOD_RCTRL, R_MOD_RGUI, R_MOD_RSHIFT};

    if (mac_mode) {
        *text = mac_text[slot];
        *bit = mac_bit[slot];
    } else {
        *text = win_text[slot];
        *bit = win_bit[slot];
    }
}

static void klor_peripheral_render(struct k_work *work) {
    struct klor_peripheral_widget *widget;

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        klor_badge_set_active(&widget->link_badge, widget_state.link_connected);

        klor_badge_set_text(&widget->bat_badge, widget_state.charging ? "CHG" : "BAT");
        char pct_buf[6];
        snprintf(pct_buf, sizeof(pct_buf), "%d%%", widget_state.battery_level);
        klor_badge_set_text(&widget->pct_badge, pct_buf);

        for (int i = 0; i < 4; i++) {
            const char *text;
            uint8_t bit;
            mod_slot(widget_state.mac_mode, i, &text, &bit);
            klor_badge_set_text(&widget->mod_badges[i], text);
            klor_badge_set_active(&widget->mod_badges[i], (widget_state.r_mods & bit) != 0);
        }
    }
}

K_WORK_DEFINE(klor_peripheral_render_work, klor_peripheral_render);

static void request_render(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &klor_peripheral_render_work);
    }
}

void klor_peripheral_widget_update_r_mods(uint8_t r_mods) {
    widget_state.r_mods = r_mods;
    request_render();
}

/* ── Event listeners ─────────────────────────────────────────────────── */

static int battery_event_cb(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    widget_state.battery_level = ev->state_of_charge;
    widget_state.charging = zmk_usb_is_powered();
    request_render();
    return ZMK_EV_EVENT_BUBBLE;
}

static int split_event_cb(const zmk_event_t *eh) {
    widget_state.link_connected = zmk_split_bt_peripheral_is_connected();
    request_render();
    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_event_cb(const zmk_event_t *eh) {
    widget_state.mac_mode =
        zmk_keymap_layer_active(KLOR_MAC_LAYER_A) || zmk_keymap_layer_active(KLOR_MAC_LAYER_B);
    request_render();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(klor_peri_battery, battery_event_cb);
ZMK_SUBSCRIPTION(klor_peri_battery, zmk_battery_state_changed);

ZMK_LISTENER(klor_peri_split, split_event_cb);
ZMK_SUBSCRIPTION(klor_peri_split, zmk_split_peripheral_status_changed);

ZMK_LISTENER(klor_peri_layer, layer_event_cb);
ZMK_SUBSCRIPTION(klor_peri_layer, zmk_layer_state_changed);

/* ── Init ────────────────────────────────────────────────────────────── */

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

int klor_peripheral_widget_init(struct klor_peripheral_widget *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);

    /* Status strip -- LINKED badge top-left, BAT/CHG + % badges top-right */
    klor_badge_create(&widget->link_badge, widget->obj, "LINKED");
    lv_obj_align(widget->link_badge.box, LV_ALIGN_TOP_LEFT, 0, 1);

    lv_obj_t *status_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_END);
    lv_obj_align(status_row, LV_ALIGN_TOP_RIGHT, 0, 1);
    klor_badge_create(&widget->bat_badge, status_row, "BAT");
    klor_badge_create(&widget->pct_badge, status_row, "100%");

    /* Rule -- above the modifier row */
    klor_rule(widget->obj, 128, 0, 27);

    /* KLOR face icon (left) + modifier badges, mirrored order (right) -- row 2 */
    widget->face_icon = lv_img_create(widget->obj);
    lv_img_set_src(widget->face_icon, &klor_face_icon);
    lv_obj_align(widget->face_icon, LV_ALIGN_TOP_LEFT, 0, 31);

    lv_obj_t *mod_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, 16, LV_FLEX_ALIGN_END);
    lv_obj_align(mod_row, LV_ALIGN_TOP_RIGHT, 0, 31);
    for (int i = 0; i < 4; i++) {
        klor_badge_create(&widget->mod_badges[i], mod_row, "SFT");
    }

    /* Rule -- below the modifier row */
    klor_rule(widget->obj, 128, 0, 49);

    sys_slist_append(&widgets, &widget->node);

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.charging = zmk_usb_is_powered();
    widget_state.link_connected = zmk_split_bt_peripheral_is_connected();
    widget_state.mac_mode =
        zmk_keymap_layer_active(KLOR_MAC_LAYER_A) || zmk_keymap_layer_active(KLOR_MAC_LAYER_B);
    widget_state.r_mods = 0;

    klor_peripheral_render(NULL);

    return 0;
}

lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget) {
    return widget->obj;
}
