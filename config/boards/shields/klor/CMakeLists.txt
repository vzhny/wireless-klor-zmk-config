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

# minimal-test branch, step 4: only klor_status_screen.c (bare "TEST" label).
# Widgets, klor_display_power.c blanking, face icon and GATT modifier sync
# all commented out -- adding back one at a time from here.
if(CONFIG_ZMK_DISPLAY)
  target_include_directories(app PRIVATE ${KLOR_SHIELD_DIR})
  target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/klor_status_screen.c)
  # target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_widgets_util.c)
  # target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_display_power.c)

  # if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_central_widget.c)
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/icons/klor_face_icon.c)
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/split/klor_modifier_sync_central.c)
  # else()
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/widgets/klor_peripheral_widget.c)
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/icons/klor_face_icon.c)
  #   target_sources(app PRIVATE ${KLOR_SHIELD_DIR}/split/klor_modifier_sync_peripheral.c)
  # endif()
endif()
