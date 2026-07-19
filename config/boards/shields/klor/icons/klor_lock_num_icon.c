#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif
#ifndef LV_ATTRIBUTE_IMG_KLOR_LOCK_NUM_ICON
#define LV_ATTRIBUTE_IMG_KLOR_LOCK_NUM_ICON
#endif
const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_IMG_KLOR_LOCK_NUM_ICON uint8_t klor_lock_num_icon_map[] = {
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xe0, 0xb5, 0xe0, 0x95, 0xe0, 0xa5, 0xe0,
  0xb5, 0xe0, 0xb4, 0x20, 0xff, 0xe0, 0x00, 0x00,
};

const lv_img_dsc_t klor_lock_num_icon = {
  .header.always_zero = 0,
  .header.w = 12,
  .header.h = 8,
  .data_size = 24,
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .data = klor_lock_num_icon_map,
};
