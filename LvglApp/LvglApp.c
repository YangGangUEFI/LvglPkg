

#include "LvglApp.h"

#include "lvgl/demos/lv_demos.h"

extern BOOLEAN  mEscExit;

void my_disp_flush(lv_display_t * disp, const lv_area_t * area, lv_color32_t * color32_p)
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


VOID
EFIAPI
TestDemo (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_STATUS                         Status;
  UINTN                              Width, Heigth;
  UINTN                              BufSize;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status)) {
    return;
  }

  lv_init();

  Width  = GraphicsOutput->Mode->Info->HorizontalResolution;
  Heigth = GraphicsOutput->Mode->Info->VerticalResolution;
  lv_display_t *display = lv_display_create(Width, Heigth);

  static lv_color32_t *buf1;
  BufSize = Width * Heigth * sizeof (lv_color32_t);
  buf1 = AllocateZeroPool (BufSize);
  lv_display_set_buffers(display, buf1, NULL, BufSize, LV_DISPLAY_RENDER_MODE_FULL);

  lv_display_set_flush_cb(display, (lv_display_flush_cb_t)my_disp_flush);

  lv_port_indev_init(display);

  lv_demo_keypad_encoder();
  // lv_demo_widgets();

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  while (1) {
    lv_tick_inc(10);
    lv_task_handler();
    if (mEscExit) {
      break;
    }
  }

  lv_deinit();

  lv_port_indev_close();

  gST->ConOut->ClearScreen (gST->ConOut);
  gST->ConOut->SetCursorPosition (gST->ConOut, 0, 0);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  return;
}


/**
  Entry point for LvglApp.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval Status         Whether this function complete successfully.

**/
EFI_STATUS
EFIAPI
LvglAppEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  TestDemo();

  return Status;
}
