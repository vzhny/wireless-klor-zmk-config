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
 * resolves keycodes/mods/layers on the central half), so the r_mods nibble,
 * the Mac/Win glyph-order flag, and the Qwerty/Colemak flag are received
 * over a custom BLE GATT characteristic the central half writes to on every
 * keycode/layer event (see split/klor_modifier_sync_*.c, ported from
 * corne's modifier_sync). That GATT write lands on the BT RX thread, not
 * the display thread, so updates go through the display work queue rather
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
#include <zmk/battery.h>
#include <zmk/usb.h>
#include <zmk/split/bluetooth/peripheral.h>

#include "klor_peripheral_widget.h"
#include "klor_widgets_util.h"
#include "../fonts/pixel_operator_mono.h"
#include "../fonts/status_icon_font.h"

LV_IMG_DECLARE(klor_face_icon);

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
    bool colemak_mode;
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
        /* Actual connection state, unlike the old hardcoded "LINKED" text --
         * that was the bug: this badge's text never changed, and its color
         * no longer inverts for being linked (see below), so it used to
         * show "LINKED" permanently regardless of whether the peripheral
         * was actually connected to anything. */
        klor_badge_set_text(&widget->link_badge, widget_state.link_connected ? "LINKED"
                                                                              : "UNLINKED");
        /* Connected is the steady/idle state -- like the central screen's BT
         * badge, this should never invert just for being linked. Only
         * mod_badges below invert, and only while actually held. */
        klor_badge_set_active(&widget->link_badge, false);

        if (widget_state.charging) {
            klor_badge_set_font(&widget->bat_badge, &status_icon_font);
            klor_badge_set_text(&widget->bat_badge, ICON_BOLT);
        } else {
            klor_badge_set_font(&widget->bat_badge, &pixel_operator_mono);
            klor_badge_set_text(&widget->bat_badge, "BAT");
        }
        char pct_buf[6];
        snprintf(pct_buf, sizeof(pct_buf), "%d%%", widget_state.battery_level);
        klor_badge_set_text(&widget->pct_badge, pct_buf);
        /* bat_badge/pct_badge just changed width (icon vs text, digit
         * count) -- status_row is LV_SIZE_CONTENT and right-anchored, so it
         * has to be re-aligned every time its content width can change, not
         * just once at init (see the field comment in
         * klor_peripheral_widget.h). */
        lv_obj_align(widget->status_row, LV_ALIGN_TOP_RIGHT, 0, 1);

        for (int i = 0; i < 4; i++) {
            const char *text;
            uint8_t bit;
            mod_slot(widget_state.mac_mode, i, &text, &bit);
            klor_badge_set_text(&widget->mod_badges[i], text);
            klor_badge_set_active(&widget->mod_badges[i], (widget_state.r_mods & bit) != 0);
        }
        lv_obj_align(widget->mod_row, LV_ALIGN_TOP_RIGHT, 0, 20);

        klor_badge_set_text(&widget->base_layer_badge, widget_state.colemak_mode ? "COLEMAK"
                                                                                  : "QWERTY");
        /* Never inverts -- an info badge, not a "something is held" badge,
         * same rule as klor_central_widget.c's layer_name_badge. */
        klor_badge_set_active(&widget->base_layer_badge, false);
    }
}

K_WORK_DEFINE(klor_peripheral_render_work, klor_peripheral_render);

static void request_render(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &klor_peripheral_render_work);
    }
}

void klor_peripheral_widget_update_mods(uint8_t payload) {
    widget_state.r_mods = payload & 0x0F;
    widget_state.mac_mode = !!(payload & BIT(4));
    widget_state.colemak_mode = !!(payload & BIT(5));
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
    /* klor_display_power.c not in this build yet (minimal-test bisection). */
    request_render();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(klor_peri_battery, battery_event_cb);
ZMK_SUBSCRIPTION(klor_peri_battery, zmk_battery_state_changed);

ZMK_LISTENER(klor_peri_split, split_event_cb);
ZMK_SUBSCRIPTION(klor_peri_split, zmk_split_peripheral_status_changed);

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

    /* Row 1 -- LINKED/UNLINKED badge top-left, BAT/CHG + % badges top-right.
     * Same ~15px badge height as the central screen (pixel_operator_mono
     * line_height 13 + 1px top/bottom padding) -- everything below this row
     * must clear y=16, or it clips into it. */
    klor_badge_create(&widget->link_badge, widget->obj, "LINKED");
    lv_obj_align(widget->link_badge.box, LV_ALIGN_TOP_LEFT, 0, 1);

    widget->status_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT, LV_FLEX_ALIGN_END);
    klor_badge_create(&widget->bat_badge, widget->status_row, "BAT");
    klor_badge_create(&widget->pct_badge, widget->status_row, "100%");
    /* Align after the children exist -- lv_obj_align() is a one-time
     * position command, not a live constraint, so aligning an empty
     * LV_SIZE_CONTENT row to TOP_RIGHT anchors it at its (zero) width at
     * call time; growing afterward as badges are added pushes the row's
     * right edge past the panel edge instead of growing leftward from it. */
    lv_obj_align(widget->status_row, LV_ALIGN_TOP_RIGHT, 0, 1);

    klor_rule(widget->obj, 128, 0, 17);

    /* Row 2 -- modifier badges only, mirrored order (right-aligned). */
    widget->mod_row =
        klor_badge_row_create(widget->obj, LV_SIZE_CONTENT, 16, LV_FLEX_ALIGN_END);
    for (int i = 0; i < 4; i++) {
        klor_badge_create(&widget->mod_badges[i], widget->mod_row, "SFT");
    }
    /* Same align-after-children rule as status_row above. */
    lv_obj_align(widget->mod_row, LV_ALIGN_TOP_RIGHT, 0, 20);

    klor_rule(widget->obj, 128, 0, 38);

    /* Row 3 -- KLOR face icon bottom-left, active base layer bottom-right. */
    widget->face_icon = lv_img_create(widget->obj);
    lv_img_set_src(widget->face_icon, &klor_face_icon);
    lv_obj_align(widget->face_icon, LV_ALIGN_TOP_LEFT, 0, 41);

    klor_badge_create(&widget->base_layer_badge, widget->obj, "QWERTY");
    lv_obj_align(widget->base_layer_badge.box, LV_ALIGN_TOP_RIGHT, 0, 41);

    sys_slist_append(&widgets, &widget->node);

    widget_state.battery_level = zmk_battery_state_of_charge();
    widget_state.charging = zmk_usb_is_powered();
    widget_state.link_connected = zmk_split_bt_peripheral_is_connected();
    /* mac_mode/colemak_mode start false (Qwerty/Win) until the first synced
     * payload arrives from central -- this half has no local layer state
     * of its own to read them from. */
    widget_state.mac_mode = false;
    widget_state.colemak_mode = false;
    widget_state.r_mods = 0;

    klor_peripheral_render(NULL);

    return 0;
}

lv_obj_t *klor_peripheral_widget_obj(struct klor_peripheral_widget *widget) {
    return widget->obj;
}
