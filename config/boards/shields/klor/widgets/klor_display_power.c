/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* KLOR OLED "kill switch".
 *
 * These are bare SSD1306 panels wired straight to the nice!nano's I2C bus,
 * not nice!view modules -- they haven't been power-profiled on this PCB, and
 * an always-on OLED draws meaningfully more current than the nice!view's
 * dedicated low-power display MCU. Until real battery numbers exist, this
 * module treats the OLED as a boot/reconnect sanity check rather than an
 * always-on status display: each half's panel stays on (as powered up) until
 * it has a live connection, then blanks; it un-blanks again the moment that
 * connection drops, including a BT profile switch on the central half (which
 * briefly drops bt_connected until the new profile finishes pairing).
 *
 * display_blanking_on()/off() (zephyr/drivers/display.h) issue the SSD1306's
 * own panel-off/on command (0xAE/0xAF) -- the same mechanism ZMK's stock
 * CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE uses, just driven by connection state here
 * instead of an idle timer. That stock idle-blank defaults to y for SSD1306
 * (app/src/display/Kconfig) and is explicitly turned off in klor_left.conf /
 * klor_right.conf -- it must stay off, since this module is meant to be the
 * only thing calling display_blanking_on/off. Both fighting over the same
 * panel is what caused a white screen on wake: the stock path also stops
 * the LVGL tick timer while blanked, so un-blanking mid-idle-cycle could
 * bring the panel visible before anything had flushed a real frame to it.
 *
 * Set CONFIG_KLOR_DISPLAY_AUTO_OFF=n to disable this entirely and keep both
 * OLEDs always on instead (wireless-corne-zmk-config's nice!view behavior).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

#include "klor_display_power.h"

#if IS_ENABLED(CONFIG_KLOR_DISPLAY_AUTO_OFF)

static const struct device *const disp_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static bool blanked;

static void apply_blanked(bool want_blanked) {
    LOG_DBG("klor_display_power: apply_blanked want=%d current=%d", want_blanked, blanked);
    if (want_blanked == blanked) {
        return;
    }
    blanked = want_blanked;
    if (blanked) {
        display_blanking_on(disp_dev);
    } else {
        display_blanking_off(disp_dev);
        /* GDDRAM isn't cleared by suspend/resume, but nothing else guarantees
         * a full repaint lands before the panel goes visible again (this can
         * fire mid-redraw, e.g. klor_central_render() calls this before it's
         * done updating badges/labels for the same state change). Force one
         * so wake never shows a stale or partial frame. */
        lv_obj_invalidate(lv_scr_act());
    }
}

/* Central: off only once both this half's BT-host link and the peripheral
 * link are up -- either dropping turns the panel back on. */
static bool c_bt_connected;
static bool c_peripheral_linked;

static void central_apply(void) {
    apply_blanked(c_bt_connected && c_peripheral_linked);
}

void klor_display_power_bt_state(bool connected) {
    LOG_DBG("klor_display_power: bt_state connected=%d", connected);
    c_bt_connected = connected;
    central_apply();
}

void klor_display_power_peripheral_link_state(bool connected) {
    LOG_DBG("klor_display_power: peripheral_link_state connected=%d", connected);
    c_peripheral_linked = connected;
    central_apply();
}

/* Peripheral: off as soon as it's linked to central. */
void klor_display_power_link_state(bool connected) {
    LOG_DBG("klor_display_power: link_state connected=%d", connected);
    apply_blanked(connected);
}

#else /* !CONFIG_KLOR_DISPLAY_AUTO_OFF */

void klor_display_power_bt_state(bool connected) {}
void klor_display_power_peripheral_link_state(bool connected) {}
void klor_display_power_link_state(bool connected) {}

#endif
