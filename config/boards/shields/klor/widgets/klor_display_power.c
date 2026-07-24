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
 * instead of an idle timer. That stock idle-blank is set to default n in
 * Kconfig.defconfig so it can't un-blank the panel on the next keypress while
 * this module still considers it connected.
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

#include "klor_display_power.h"

#if IS_ENABLED(CONFIG_KLOR_DISPLAY_AUTO_OFF)

static const struct device *const disp_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static bool blanked;

static void apply_blanked(bool want_blanked) {
    if (want_blanked == blanked) {
        return;
    }
    blanked = want_blanked;
    if (blanked) {
        display_blanking_on(disp_dev);
    } else {
        display_blanking_off(disp_dev);
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
    c_bt_connected = connected;
    central_apply();
}

void klor_display_power_peripheral_link_state(bool connected) {
    c_peripheral_linked = connected;
    central_apply();
}

/* Peripheral: off as soon as it's linked to central. */
void klor_display_power_link_state(bool connected) { apply_blanked(connected); }

#else /* !CONFIG_KLOR_DISPLAY_AUTO_OFF */

void klor_display_power_bt_state(bool connected) {}
void klor_display_power_peripheral_link_state(bool connected) {}
void klor_display_power_link_state(bool connected) {}

#endif
