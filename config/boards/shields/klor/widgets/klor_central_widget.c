/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* KLOR "master side" (left/central) OLED screen.
 *
 * Layer name (monospace) + a 1-9 active-layer badge row recreate what QMK's
 * klor.c showed (current layer, at a glance); everything else here (BT/BAT/%
 * status strip, live modifier badges, layer-number badges) is new, styled to
 * match wireless-corne-zmk-config's status strip and this build's badge
 * widget (klor_widgets_util.c) rather than reusing any bitmap glyphs.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/usb.h>
#include <zmk/hid.h>

#include "klor_central_widget.h"
#include "klor_widgets_util.h"

LV_IMG_DECLARE(klor_face_icon);

/* &tog'd base-layer indices from klor.keymap: 1 = Qwerty (Mac), 3 = Colemak-DH (Mac) */
#define KLOR_MAC_LAYER_A 1
#define KLOR_MAC_LAYER_B 3

/* Standard HID modifier byte: bit0=LCtrl,bit1=LShift,bit2=LAlt,bit3=LGui */
#define MOD_LCTRL BIT(0)
#define MOD_LSHIFT BIT(1)
#define MOD_LALT BIT(2)
#define MOD_LGUI BIT(3)

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct klor_central_state {
    const char *layer_label;
    int active_layer;
    bool mac_mode;
    uint8_t battery_level;
    bool charging;
    bool bt_connected;
    int profile_index;
    uint32_t mods;
};

/* ── BT connecting-dots animation ────────────────────────────────────── */

static bool bt_flash_on = true;
static uint8_t connecting_dots = 1; /* 1..3, advances each 500ms tick while searching */
static bool bt_was_connected = true;
static struct klor_central_state last_state;

static void bt_flash_work_cb(struct k_work *work);
static K_WORK_DEFINE(bt_flash_work, bt_flash_work_cb);

static inline void submit_bt_flash_work(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &bt_flash_work);
    }
}

static void bt_flash_timer_cb(struct k_timer *timer) { submit_bt_flash_work(); }
static K_TIMER_DEFINE(bt_flash_timer, bt_flash_timer_cb, NULL);

/* Left-side modifier slot order, matching wireless-corne-zmk-config's
 * render_mod_canvas(): Win = SFT,CTL,GUI,ALT / Mac = SFT,CMD,CTL,OPT */
static void mod_slot(bool mac_mode, int slot, const char **text, uint32_t *bit) {
    static const char *const win_text[4] = {"SFT", "CTL", "GUI", "ALT"};
    static const uint32_t win_bit[4] = {MOD_LSHIFT, MOD_LCTRL, MOD_LGUI, MOD_LALT};
    static const char *const mac_text[4] = {"SFT", "CMD", "CTL", "OPT"};
    static const uint32_t mac_bit[4] = {MOD_LSHIFT, MOD_LGUI, MOD_LCTRL, MOD_LALT};

    if (mac_mode) {
        *text = mac_text[slot];
        *bit = mac_bit[slot];
    } else {
        *text = win_text[slot];
        *bit = win_bit[slot];
    }
}

static void klor_central_render(struct klor_central_state state) {
    struct klor_central_widget *widget;

    last_state = state;

    if (state.bt_connected != bt_was_connected) {
        bt_was_connected = state.bt_connected;
        if (state.bt_connected) {
            k_timer_stop(&bt_flash_timer);
            bt_flash_on = true;
        } else {
            bt_flash_on = true;  /* start visible so first visible frame shows the badge */
            connecting_dots = 1; /* restart dot animation at "." */
            k_timer_start(&bt_flash_timer, K_MSEC(500), K_MSEC(500));
        }
    }

    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        if (state.bt_connected) {
            klor_badge_set_active(&widget->bt_badge, true);
            char profile_buf[2];
            snprintf(profile_buf, sizeof(profile_buf), "%d", state.profile_index);
            klor_badge_set_text(&widget->profile_badge, profile_buf);
            klor_badge_set_active(&widget->profile_badge, true);
        } else {
            klor_badge_set_active(&widget->bt_badge, bt_flash_on);
            char dots_buf[4];
            snprintf(dots_buf, sizeof(dots_buf), "%.*s", connecting_dots, "...");
            klor_badge_set_text(&widget->profile_badge, dots_buf);
            klor_badge_set_active(&widget->profile_badge, bt_flash_on);
        }

        klor_badge_set_text(&widget->bat_badge, state.charging ? "CHG" : "BAT");
        char pct_buf[6];
        snprintf(pct_buf, sizeof(pct_buf), "%d%%", state.battery_level);
        klor_badge_set_text(&widget->pct_badge, pct_buf);

        if (state.layer_label != NULL && strlen(state.layer_label) > 0) {
            lv_label_set_text(widget->layer_label, state.layer_label);
        } else {
            lv_label_set_text(widget->layer_label, "");
        }

        for (int i = 0; i < 4; i++) {
            const char *text;
            uint32_t bit;
            mod_slot(state.mac_mode, i, &text, &bit);
            klor_badge_set_text(&widget->mod_badges[i], text);
            klor_badge_set_active(&widget->mod_badges[i], (state.mods & bit) != 0);
        }

        for (int i = 0; i < 9; i++) {
            klor_badge_set_active(&widget->layer_badges[i], state.active_layer == i);
        }
    }
}

static void bt_flash_work_cb(struct k_work *work) {
    bt_flash_on = !bt_flash_on;
    connecting_dots = (connecting_dots % 3) + 1;
    klor_central_render(last_state);
}

static struct klor_central_state klor_central_get_state(const zmk_event_t *_eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();

    return (struct klor_central_state){
        .layer_label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index)),
        .active_layer = index,
        .mac_mode = zmk_keymap_layer_active(KLOR_MAC_LAYER_A) ||
                    zmk_keymap_layer_active(KLOR_MAC_LAYER_B),
        .battery_level = zmk_battery_state_of_charge(),
        .charging = zmk_usb_is_powered(),
        .bt_connected = zmk_ble_active_profile_is_connected(),
        .profile_index = zmk_ble_active_profile_index(),
        .mods = zmk_hid_get_explicit_mods(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_klor_central, struct klor_central_state,
                            klor_central_render, klor_central_get_state)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_layer_state_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_battery_state_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_ble_active_profile_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_keycode_state_changed)

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

    /* Status strip -- BT + profile badges top-left, BAT/CHG + % badges top-right */
    lv_obj_t *bt_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_START);
    lv_obj_align(bt_row, LV_ALIGN_TOP_LEFT, 0, 1);
    klor_badge_create(&widget->bt_badge, bt_row, "BT");
    klor_badge_create(&widget->profile_badge, bt_row, "X");

    lv_obj_t *status_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_END);
    lv_obj_align(status_row, LV_ALIGN_TOP_RIGHT, 0, 1);
    klor_badge_create(&widget->bat_badge, status_row, "BAT");
    klor_badge_create(&widget->pct_badge, status_row, "100%");

    /* Layer name (monospace) -- row 0 */
    widget->layer_label = lv_label_create(widget->obj);
    lv_obj_set_style_text_font(widget->layer_label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_align(widget->layer_label, LV_ALIGN_TOP_LEFT, 0, 16);

    /* Rule -- row 1 */
    klor_rule(widget->obj, 128, 0, 27);

    /* Modifier badges (left) + KLOR face icon (right) -- row 2 */
    lv_obj_t *mod_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, 16, LV_FLEX_ALIGN_START);
    lv_obj_align(mod_row, LV_ALIGN_TOP_LEFT, 0, 31);
    for (int i = 0; i < 4; i++) {
        klor_badge_create(&widget->mod_badges[i], mod_row, "SFT");
    }

    widget->face_icon = lv_img_create(widget->obj);
    lv_img_set_src(widget->face_icon, &klor_face_icon);
    lv_obj_align(widget->face_icon, LV_ALIGN_TOP_RIGHT, 0, 31);

    /* Rule -- row 3 */
    klor_rule(widget->obj, 128, 0, 49);

    /* Layer number badges 1-9 -- row 4, replaces the old lock-key row */
    lv_obj_t *layer_row = klor_badge_row_create(widget->obj, 128, 11, LV_FLEX_ALIGN_START);
    lv_obj_align(layer_row, LV_ALIGN_TOP_LEFT, 0, 53);
    for (int i = 0; i < 9; i++) {
        char buf[2];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        klor_badge_create(&widget->layer_badges[i], layer_row, buf);
    }

    sys_slist_append(&widgets, &widget->node);

    widget_klor_central_init();

    return 0;
}

lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget) { return widget->obj; }
