# The zmk-actions CI workflow copies ZMK_CONFIG into an isolated temp
# directory for the actual build, and this file ends up getting included
# from both that copy and the original checkout (west's own module
# discovery vs. ZMK_CONFIG-driven discovery landing on different physical
# paths for what's otherwise identical content) -- without this guard,
# every source here gets compiled and linked twice, causing duplicate
# symbol errors at link time.
get_property(klor_cmakelists_included GLOBAL PROPERTY KLOR_CMAKELISTS_INCLUDED)
if(klor_cmakelists_included)
  return()
endif()
set_property(GLOBAL PROPERTY KLOR_CMAKELISTS_INCLUDED TRUE)

set(KLOR_SHIELD_DIR ${CMAKE_CURRENT_LIST_DIR})

if(CONFIG_ZMK_DISPLAY)
  target_include_directories(app PRIVATE ${KLOR_SHIELD_DIR})
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/klor_status_screen.c)
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_widgets_util.c)
  # Ported from wireless-corne-zmk-config: PixelOperatorMono (crisp on a
  # 1bpp monochrome panel, unlike antialiased Montserrat) plus a Nerd Font
  # icon subset for future glyph use. icon_font.c isn't used by any widget
  # yet -- just wired in and available.
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/fonts/pixel_operator_mono.c)
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/fonts/pixel_operator_mono_large.c)
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/fonts/icon_font.c)
  # Same Nerd Font source, converted separately for status-row-sized glyphs
  # (18px vs icon_font.c's 24px) -- only the bolt/charging icon is used so
  # far, see status_icon_font.h.
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/fonts/status_icon_font.c)
  # target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_display_power.c)

  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_central_widget.c)
    # target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/icons/klor_face_icon.c)
    target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/split/klor_modifier_sync_central.c)
  else()
    target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_peripheral_widget.c)
    # klor_peripheral_widget.c's face icon needs this symbol to link.
    target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/icons/klor_face_icon.c)
    target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/split/klor_modifier_sync_peripheral.c)
  endif()
endif()
