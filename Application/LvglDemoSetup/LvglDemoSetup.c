/** @file
  This sample application bases on LvglLib.

  Copyright (c) 2024, Yang Gang. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiSetupCommon.h"

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_group_t * g;
static lv_obj_t * tv;
static lv_obj_t * t1;
static lv_obj_t * t2;


static void BiosInfoCreate(lv_obj_t * parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);

    UefiBasicInfo (parent);
}


static void BootManagerCreate(lv_obj_t * parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    UefiBootOptions (parent);
}


VOID
EFIAPI
LvglUefiDemoSetup (
  VOID
  )
{
    g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t * indev = NULL;
    for(;;) {
        indev = lv_indev_get_next(indev);
        if(!indev) {
            break;
        }

        lv_indev_type_t indev_type = lv_indev_get_type(indev);
        if(indev_type == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(indev, g);
        }

        if(indev_type == LV_INDEV_TYPE_POINTER) {
            lv_indev_set_group(indev, g);
        }
    }

    tv = lv_tabview_create(lv_screen_active());

    t1 = lv_tabview_add_tab(tv, "BIOS Info");
    lv_tabview_set_tab_text_font (tv, 0, &lv_font_montserrat_20);
    t2 = lv_tabview_add_tab(tv, "Boot Manager");
    lv_tabview_set_tab_text_font (tv, 1, &lv_font_montserrat_20);

    BiosInfoCreate (t1);
    BootManagerCreate (t2);
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI EfiImageLogo.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (UefiLvglInit() == EFI_SUCCESS) {
    Status = UefiLvglAppRegister (LvglUefiDemoSetup);

    if (!EFI_ERROR (Status)) {
      UefiLvglDeinit();
    }
  }

  return EFI_SUCCESS;
}
