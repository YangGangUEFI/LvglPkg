
#include "LvglLibCommon.h"

#include <Library/LvglLib.h>

extern BOOLEAN  mEscExit;

BOOLEAN  mTickSupport = FALSE;
STATIC BOOLEAN  mUefiLvglInitDone = FALSE;


void efi_disp_flush(lv_display_t * disp, const lv_area_t * area, lv_color32_t * color32_p)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_STATUS                         Status;
  UINTN                              Width, Heigth;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status)) {
    return;
  }

  Width = area->x2 - area->x1 + 1;
  Heigth = area->y2 - area->y1 + 1;

  Status =  GraphicsOutput->Blt (
                              GraphicsOutput,
                              (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)color32_p,
                              EfiBltBufferToVideo,
                              0,
                              0,
                              (UINTN)area->x1,
                              (UINTN)area->y1,
                              Width,
                              Heigth,
                              0
                              );


  lv_display_flush_ready(disp);
}


#if LV_USE_LOG
static void efi_lv_log_print(lv_log_level_t level, const char * buf)
{
    static const int priority[LV_LOG_LEVEL_NUM] = {
        DEBUG_VERBOSE, DEBUG_INFO, DEBUG_WARN, DEBUG_ERROR, DEBUG_INFO, DEBUG_INIT
    };

    DebugPrint (priority[level], "[LVGL] %a\n", buf);
}
#endif


static uint32_t tick_get_cb(void)
{
  return (UINT32) DivU64x32 (GetTimeInNanoSecond (GetPerformanceCounter()), 1000 * 1000);
}

VOID
EFIAPI
UefiLvglTickInit (
  VOID
  )
{
  if (GetPerformanceCounter()) {
    mTickSupport = TRUE;
    lv_tick_set_cb(tick_get_cb);
  }
}


EFI_STATUS
EFIAPI
UefiLvglInit (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_STATUS                         Status;
  UINTN                              Width, Heigth;
  UINTN                              BufSize;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status)) {
    return EFI_UNSUPPORTED;
  }

  lv_init();

  UefiLvglTickInit();

#if LV_USE_LOG
  lv_log_register_print_cb (efi_lv_log_print);
#endif

  Width  = GraphicsOutput->Mode->Info->HorizontalResolution;
  Heigth = GraphicsOutput->Mode->Info->VerticalResolution;
  lv_display_t *display = lv_display_create(Width, Heigth);

  static lv_color32_t *buf1;
  static lv_color32_t *buf2;
  BufSize = Width * Heigth * sizeof (lv_color32_t);
  buf1 = malloc (BufSize);
  buf2 = malloc (BufSize);
  if (!buf1 || !buf2) {
    DebugPrint (DEBUG_ERROR, "Cannot init lv_display, deinit and return.\n");
    lv_deinit();
    return EFI_DEVICE_ERROR;
  }

  lv_display_set_buffers(display, buf1, buf2, BufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, (lv_display_flush_cb_t)efi_disp_flush);

  lv_port_indev_init(display);

  mUefiLvglInitDone = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiLvglDeinit (
  VOID
  )
{

  lv_deinit();

  lv_port_indev_close();

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiLvglAppRegister (
  IN EFI_LVGL_APP_FUNCTION AppRegister
  )
{
  if (!mUefiLvglInitDone) {
    if (UefiLvglInit() != EFI_SUCCESS) {
      return EFI_UNSUPPORTED;
    }
  }

  if (AppRegister != NULL) {
    gST->ConOut->ClearScreen (gST->ConOut);
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);

    // call user GUI APP
    AppRegister();

    while (1) {
      if (mEscExit) {
        break;
      }

      lv_timer_handler();

      gBS->Stall (10 * 1000);
      if (!mTickSupport) {
        lv_tick_inc(10);
      }
    }
  } else {
    UefiLvglDeinit();
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
