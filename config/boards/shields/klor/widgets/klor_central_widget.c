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
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/usb.h>

#include "klor_central_widget.h"
#include "klor_widgets_util.h"
#include "klor_display_power.h"
#include "../split/klor_modifier_sync.h"

LV_IMG_DECLARE(klor_face_icon);

/* &tog'd base-layer indices from klor.keymap: 1 = Qwerty (Mac), 3 = Colemak-DH (Mac) */
#define KLOR_MAC_LAYER_A 1
#define KLOR_MAC_LAYER_B 3

/* Standard HID modifier byte layout - bits 0-3 are the left-hand mods this
 * half's own display reads, bits 4-7 are the right-hand mods forwarded to
 * the peripheral (see klor_modifier_sync_central.c). */
#define MOD_LCTRL BIT(0)
#define MOD_LSHIFT BIT(1)
#define MOD_LALT BIT(2)
#define MOD_LGUI BIT(3)
#define MOD_RCTRL BIT(4)
#define MOD_RSHIFT BIT(5)
#define MOD_RALT BIT(6)
#define MOD_RGUI BIT(7)

/* What the mod cells actually display - built entirely from watching raw
 * key positions (see the shadow-tracking section below), not real HID mod
 * state. Every modifier in klor.keymap is bound through one of the 8
 * tracked hold-tap positions, so this alone is a complete picture of what
 * should be lit - and it sidesteps a confirmed-on-hardware bug (see
 * wireless-corne-zmk-config@75b8ca3) where reading the real HID snapshot
 * left a mod cell stuck lit after releasing a modifier held in isolation,
 * with no other key pressed to force a resync. Display-only, 8-bit HID
 * shape (bits 0-3 left, 4-7 right) - never fed back into actual HID
 * output. */
static uint8_t shadow_mods;

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct klor_central_state {
    const char *layer_label;
    int active_layer;
    bool mac_mode;
    uint8_t battery_level;
    bool charging;
    bool bt_connected;
    int profile_index;
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
    klor_display_power_bt_state(state.bt_connected);

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
            klor_badge_set_active(&widget->mod_badges[i], (shadow_mods & bit) != 0);
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

/* ── Modifier shadow-tracking (display-only) ──────────────────────────── *
 *
 * Real HID mod state only updates once ZMK's hold-tap logic actually
 * decides tap vs hold, which for hml/hmr/thm's "balanced" flavor can lag
 * well behind the physical press (interrupt-release rule). This watches
 * raw key presses on the known modifier positions directly (fired the
 * instant a physical key goes down/up, before hold-tap/combo logic
 * resolves it) and manually lights up the display after that position's
 * own tapping-term-ms, independent of whatever ZMK's real hold-tap
 * decision eventually is - approximating "hold long enough and it's
 * obviously a hold" the same way a person would judge it by eye. Ported
 * from wireless-corne-zmk-config@dd9443c/75b8ca3.
 *
 * Deliberate approximation, not real HID state, and it can disagree with
 * it - e.g. a quick type-through that resolves HOLD via balanced's
 * interrupt-release rule before this timer fires won't light up here, and
 * a position that's also part of a combo (10 shares combo_esc; 34 shares
 * combo_undo and combo_caps_word; 35 shares combo_caps_word) can show a
 * slightly late shadow light if that combo doesn't end up firing. Accepted
 * trade-offs for a real-time-*feeling* indicator, not a perfectly accurate
 * one - hml/hmr/thm's actual tap/hold behavior is untouched by any of
 * this.
 *
 * Position -> logical mod, from klor.keymap's homerow/thumb bindings
 * (KLOR_MAC_LAYER_A/B swap Ctrl<->Gui on the homerow only; positions 12
 * and 17 don't swap; update this table if those bindings ever move): */

struct shadow_mod_slot {
    uint32_t position;
    uint32_t tapping_term_ms;
    bool swaps_with_mac; /* true for the two Ctrl/Gui-swap positions per hand */
    uint8_t bit_win;     /* bit when !swaps_with_mac, or when swaps_with_mac && !is_mac */
    uint8_t bit_mac;     /* bit when swaps_with_mac && is_mac (unused otherwise) */
    bool held;
    bool fired;
    uint8_t applied_bit;
    struct k_work_delayable work;
};

static void shadow_slot_timeout(struct k_work *work);

static struct shadow_mod_slot shadow_slots[] = {
    /* Left homerow: A/S/D (Ctrl/Gui swap with Mac layers, Alt fixed) */
    {.position = 10,
     .tapping_term_ms = 280,
     .swaps_with_mac = true,
     .bit_win = MOD_LCTRL,
     .bit_mac = MOD_LGUI},
    {.position = 11,
     .tapping_term_ms = 280,
     .swaps_with_mac = true,
     .bit_win = MOD_LGUI,
     .bit_mac = MOD_LCTRL},
    {.position = 12, .tapping_term_ms = 280, .swaps_with_mac = false, .bit_win = MOD_LALT},
    /* Right homerow: K/L/; (mirrors the left hand's swap) */
    {.position = 17, .tapping_term_ms = 280, .swaps_with_mac = false, .bit_win = MOD_RALT},
    {.position = 18,
     .tapping_term_ms = 280,
     .swaps_with_mac = true,
     .bit_win = MOD_RGUI,
     .bit_mac = MOD_RCTRL},
    {.position = 19,
     .tapping_term_ms = 280,
     .swaps_with_mac = true,
     .bit_win = MOD_RCTRL,
     .bit_mac = MOD_RGUI},
    /* Thumb shift keys - never swap with Mac/Win */
    {.position = 34, .tapping_term_ms = 200, .swaps_with_mac = false, .bit_win = MOD_LSHIFT},
    {.position = 35, .tapping_term_ms = 200, .swaps_with_mac = false, .bit_win = MOD_RSHIFT},
};
#define SHADOW_SLOT_COUNT ARRAY_SIZE(shadow_slots)

static void shadow_render_work_cb(struct k_work *work) { klor_central_render(last_state); }
static K_WORK_DEFINE(shadow_render_work, shadow_render_work_cb);

static inline void submit_shadow_render(void) {
    if (zmk_display_is_initialized()) {
        k_work_submit_to_queue(zmk_display_work_q(), &shadow_render_work);
    }
}

static void shadow_slot_timeout(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct shadow_mod_slot *slot = CONTAINER_OF(dwork, struct shadow_mod_slot, work);
    if (!slot->held) {
        return; /* released before the timer fired - a tap, not a hold */
    }
    slot->applied_bit =
        (slot->swaps_with_mac && last_state.mac_mode) ? slot->bit_mac : slot->bit_win;
    slot->fired = true;
    shadow_mods |= slot->applied_bit;
    submit_shadow_render();
    klor_modifier_sync_notify_mods_changed();
}

static int position_event_cb(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (size_t i = 0; i < SHADOW_SLOT_COUNT; i++) {
        struct shadow_mod_slot *slot = &shadow_slots[i];
        if (slot->position != ev->position) {
            continue;
        }
        if (ev->state) {
            slot->held = true;
            slot->fired = false;
            k_work_schedule(&slot->work, K_MSEC(slot->tapping_term_ms));
        } else {
            slot->held = false;
            k_work_cancel_delayable(&slot->work);
            if (slot->fired) {
                slot->fired = false;
                shadow_mods &= ~slot->applied_bit;
                submit_shadow_render();
                klor_modifier_sync_notify_mods_changed();
            }
        }
        break;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(klor_central_position, position_event_cb);
ZMK_SUBSCRIPTION(klor_central_position, zmk_position_state_changed);

/* Shadow-tracked mods, 8-bit HID shape (bits 0-3 left, 4-7 right) - used by
 * klor_modifier_sync_central.c to forward the right-hand nibble to the
 * peripheral. */
uint8_t klor_central_widget_get_display_mods(void) { return shadow_mods; }

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
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_klor_central, struct klor_central_state,
                            klor_central_render, klor_central_get_state)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_layer_state_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_battery_state_changed)
ZMK_SUBSCRIPTION(widget_klor_central, zmk_ble_active_profile_changed)

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

    for (size_t i = 0; i < SHADOW_SLOT_COUNT; i++) {
        k_work_init_delayable(&shadow_slots[i].work, shadow_slot_timeout);
    }

    widget_klor_central_init();

    return 0;
}

lv_obj_t *klor_central_widget_obj(struct klor_central_widget *widget) { return widget->obj; }
