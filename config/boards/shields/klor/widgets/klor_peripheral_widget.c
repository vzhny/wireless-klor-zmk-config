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
#include <zmk/events/usb_conn_state_changed.h>
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

        for (int i = 0; i < 4; i++) {
            const char *text;
            uint8_t bit;
            mod_slot(widget_state.mac_mode, i, &text, &bit);
            klor_badge_set_text(&widget->mod_badges[i], text);
            klor_badge_set_active(&widget->mod_badges[i], (widget_state.r_mods & bit) != 0);
        }

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

/* Without this, charging only got picked up inside battery_event_cb, which
 * only fires on a battery tick -- plugging into USB-C doesn't relabel the
 * BAT badge to the bolt icon on its own, and with no battery installed
 * (ADC has nothing to report) that tick may not come at all. */
static int usb_event_cb(const zmk_event_t *eh) {
    widget_state.charging = zmk_usb_is_powered();
    request_render();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(klor_peri_battery, battery_event_cb);
ZMK_SUBSCRIPTION(klor_peri_battery, zmk_battery_state_changed);

ZMK_LISTENER(klor_peri_split, split_event_cb);
ZMK_SUBSCRIPTION(klor_peri_split, zmk_split_peripheral_status_changed);

ZMK_LISTENER(klor_peri_usb, usb_event_cb);
ZMK_SUBSCRIPTION(klor_peri_usb, zmk_usb_conn_state_changed);

/* ── Init ────────────────────────────────────────────────────────────── */

static lv_obj_t *klor_rule(lv_obj_t *parent, lv_coord_t w, lv_coord_t x, lv_coord_t y) {
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_size(line, w, 1);
    /* Physical panel has inversion-on set (klor_common.dtsi), which flips
     * these explicit fill colors same as klor_badge_create() -- this was
     * lv_color_white() here, which nets out to a physically BLACK line
     * (invisible against the idle black background). klor_central_widget.c's
     * copy of this same helper already had the correct swap; this one
     * didn't, which is why the right screen's separators were invisible. */
    lv_obj_set_style_bg_color(line, lv_color_black(), LV_PART_MAIN);
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

    /* Fixed-width (== panel width), not LV_SIZE_CONTENT -- a content-sized
     * row's own box has to be re-aligned every time a child's width changes
     * (bat_badge: BAT vs the bolt icon; pct_badge: digit count), since
     * lv_obj_align() is a one-time position command, not a live constraint.
     * A row that's already as wide as the panel never needs to move again;
     * LV_FLEX_ALIGN_END packs its children flush against the right edge
     * regardless of their combined width, so this is a one-time alignment
     * that stays correct through every future content change. The row's own
     * background is transparent, so the unused space to its left is
     * invisible and doesn't overlap link_badge visually. */
    lv_obj_t *status_row =
        klor_badge_row_create(widget->obj, 128, LV_SIZE_CONTENT, LV_FLEX_ALIGN_END);
    klor_badge_create(&widget->bat_badge, status_row, "BAT");
    klor_badge_create(&widget->pct_badge, status_row, "100%");
    lv_obj_align(status_row, LV_ALIGN_TOP_RIGHT, 0, 1);

    /* y=19 (not 17) leaves a bit more breathing room below the status
     * badges than a bare 1px gap -- matches klor_central_widget.c. */
    klor_rule(widget->obj, 128, 0, 19);

    /* Row 2 -- modifier badges only, mirrored order (right-aligned). Same
     * fixed-width-row reasoning as status_row above -- this is also what
     * was silently dropping 3 of the 4 badges off-panel before: a
     * LV_SIZE_CONTENT row anchored TOP_RIGHT still needs its final content
     * width to compute where its left edge lands, and by the time
     * lv_obj_align() ran here, LVGL's flex layout for the just-added
     * badges hadn't necessarily settled yet. A fixed-width row sidesteps
     * that entirely -- its own position never depends on its children. */
    lv_obj_t *mod_row = klor_badge_row_create(widget->obj, 128, 16, LV_FLEX_ALIGN_END);
    for (int i = 0; i < 4; i++) {
        klor_badge_create(&widget->mod_badges[i], mod_row, "SFT");
    }
    lv_obj_align(mod_row, LV_ALIGN_TOP_RIGHT, 0, 22);

    klor_rule(widget->obj, 128, 0, 40);

    /* Row 3 -- KLOR face icon bottom-left, active base layer bottom-right. */
    widget->face_icon = lv_img_create(widget->obj);
    lv_img_set_src(widget->face_icon, &klor_face_icon);
    lv_obj_align(widget->face_icon, LV_ALIGN_TOP_LEFT, 0, 43);

    klor_badge_create(&widget->base_layer_badge, widget->obj, "QWERTY");
    lv_obj_align(widget->base_layer_badge.box, LV_ALIGN_TOP_RIGHT, 0, 43);

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
