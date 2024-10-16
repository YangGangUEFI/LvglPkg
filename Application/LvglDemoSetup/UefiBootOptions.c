#include "UefiSetupCommon.h"
#include <Library/UefiBootManagerLib.h>

static lv_obj_t * BootOptionsList;

static lv_obj_t * currentButton = NULL;

EFI_BOOT_MANAGER_LOAD_OPTION  *mLoadOptions;
UINTN                         mLoadOptionCount;

static void LoadOptionsDelEventHandler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_DELETE) {
      EfiBootManagerFreeLoadOptions (mLoadOptions, mLoadOptionCount);
    }
}

static void BootOptionBootEventHandler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        LV_LOG_USER("Boot %s", lv_list_get_button_text(BootOptionsList, obj));

        EFI_BOOT_MANAGER_LOAD_OPTION *CurrentBootOption;

        CurrentBootOption = lv_event_get_user_data (e);

        if (CurrentBootOption) {
          EfiBootManagerBoot (CurrentBootOption);
        }

        if (CurrentBootOption->Status == EFI_SUCCESS) {
          lv_display_t * disp = lv_display_get_default();
          lv_obj_invalidate(disp->sys_layer);
        }
    }
}


VOID
UefiBootOptions (
  lv_obj_t * parent
  )
{
  CHAR8                         ButtonText[256+1];
  UINTN                         Index;

  mLoadOptions = EfiBootManagerGetLoadOptions (&mLoadOptionCount, LoadOptionTypeBoot);

  if (mLoadOptionCount == 0) {
    lv_obj_t * InfoLabel = lv_label_create(parent);
    lv_label_set_text(InfoLabel, "No valid boot options!");
    lv_obj_set_style_text_font(InfoLabel, &lv_font_montserrat_18, 0);
    return;
  } else {
    lv_obj_t * InfoLabel = lv_label_create(parent);
    lv_label_set_text(InfoLabel, "Select boot option to boot OS");
    lv_obj_set_style_text_font(InfoLabel, &lv_font_montserrat_18, 0);
  }

  BootOptionsList = lv_list_create(parent);
  lv_obj_set_size(BootOptionsList, lv_pct(95), lv_pct(90));

  lv_obj_t * btn;
  for(Index = 0; Index < mLoadOptionCount; Index++) {
    ZeroMem (ButtonText, sizeof (ButtonText));
    AsciiSPrint (ButtonText, sizeof (ButtonText), "BootOption: %s", mLoadOptions[Index].Description);
    btn = lv_list_add_button (BootOptionsList, NULL, ButtonText);
    lv_obj_set_width(btn, lv_pct(80));
    lv_obj_add_flag(btn, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(btn, BootOptionBootEventHandler, LV_EVENT_CLICKED, &mLoadOptions[Index]);
  }

  currentButton = lv_obj_get_child(BootOptionsList, 0);
  lv_obj_add_state(currentButton, LV_STATE_CHECKED);

  lv_obj_add_event_cb(BootOptionsList, LoadOptionsDelEventHandler, LV_EVENT_DELETE, NULL);
}