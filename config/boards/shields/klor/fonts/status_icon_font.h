#pragma once

#include <lvgl.h>

/* Ported from wireless-corne-zmk-config's status_icon_font (same source
 * font/conversion, already verified on real hardware there) - converted
 * from JetBrainsMonoNLNerdFont-Regular.ttf via lv_font_conv (--bpp 1
 * --size 18), a separate conversion from icon_font.h's, sized for status-
 * row badges rather than the 28px modifier cells.
 *
 * Only ICON_BOLT is wired into a widget so far (klor_central_widget.c /
 * klor_peripheral_widget.c's charging indicator) - the BT/battery-level
 * glyphs are carried over unused, available if those badges ever want an
 * icon instead of text. */
extern const lv_font_t status_icon_font;

#define ICON_BT              "\xef\x8a\x94"     /* U+F294  fa-bluetooth_b */
#define ICON_BATTERY_FULL    "\xef\x89\x80"     /* U+F240  fa-battery_full        76-100% */
#define ICON_BATTERY_3_4     "\xef\x89\x81"     /* U+F241  fa-battery_three_quarters 51-75% */
#define ICON_BATTERY_HALF    "\xef\x89\x82"     /* U+F242  fa-battery_half        26-50% */
#define ICON_BATTERY_QUARTER "\xef\x89\x83"     /* U+F243  fa-battery_quarter      6-25% */
#define ICON_BATTERY_EMPTY   "\xef\x89\x84"     /* U+F244  fa-battery_empty        1-5%, blinks */
#define ICON_BOLT            "\xef\x83\xa7"     /* U+F0E7  fa-flash (Font Awesome's bolt icon) */
