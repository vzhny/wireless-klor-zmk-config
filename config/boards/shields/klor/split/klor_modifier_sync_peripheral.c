/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "klor_modifier_sync.h"
#include "../widgets/klor_peripheral_widget.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct bt_uuid_128 mod_svc_uuid = BT_UUID_INIT_128(BT_UUID_KLOR_MOD_SVC_VAL);
static struct bt_uuid_128 mod_char_uuid = BT_UUID_INIT_128(BT_UUID_KLOR_MOD_CHAR_VAL);

static ssize_t write_mod_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                               const void *buf, uint16_t len, uint16_t offset, uint8_t flags) {
    if (len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }
    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    klor_peripheral_widget_update_r_mods(*(const uint8_t *)buf);
    return len;
}

BT_GATT_SERVICE_DEFINE(klor_mod_svc, BT_GATT_PRIMARY_SERVICE(&mod_svc_uuid),
                       BT_GATT_CHARACTERISTIC(&mod_char_uuid,
                                              BT_GATT_CHRC_WRITE_WITHOUT_RESP |
                                                  BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE, NULL, write_mod_state, NULL), );
