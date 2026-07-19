/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "klor_widgets_util.h"

lv_obj_t *klor_badge_row_create(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                lv_flex_align_t row_align) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, w, h);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(row, 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, row_align, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    return row;
}

void klor_badge_create(struct klor_badge *badge, lv_obj_t *parent, const char *text) {
    badge->box = lv_obj_create(parent);
    lv_obj_set_size(badge->box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(badge->box, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(badge->box, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(badge->box, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(badge->box, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(badge->box, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge->box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(badge->box, LV_OBJ_FLAG_SCROLLABLE);

    badge->label = lv_label_create(badge->box);
    lv_obj_set_style_text_color(badge->label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(badge->label, &lv_font_montserrat_8, LV_PART_MAIN);
    lv_label_set_text(badge->label, text);
    lv_obj_center(badge->label);
}

void klor_badge_set_text(struct klor_badge *badge, const char *text) {
    lv_label_set_text(badge->label, text);
}

void klor_badge_set_active(struct klor_badge *badge, bool active) {
    lv_obj_set_style_bg_color(badge->box, active ? lv_color_white() : lv_color_black(),
                              LV_PART_MAIN);
    lv_obj_set_style_text_color(badge->label, active ? lv_color_black() : lv_color_white(),
                                LV_PART_MAIN);
}
