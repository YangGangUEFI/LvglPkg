/** @file
  This sample application bases on LvglLib.

  Copyright (c) 2024, Yang Gang. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/PlatformLogo.h>

#include <Library/LvglLib.h>

LV_ATTRIBUTE_MEM_ALIGN UINT8 *mLogoData = NULL;

VOID
EFIAPI
DisplayUefiLogo (
  EFI_IMAGE_INPUT *EfiImageLogo
  )
{
  UINT32 Width, Height, Stride, DataSize;

  Width = EfiImageLogo->Width;
  Height = EfiImageLogo->Height;
  Stride = EfiImageLogo->Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  DataSize = EfiImageLogo->Width * EfiImageLogo->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

  mLogoData = AllocateZeroPool (DataSize);
  CopyMem (mLogoData, EfiImageLogo->Bitmap, DataSize);

  for (UINTN Index = 3; Index < DataSize; Index += 4) {
    mLogoData[Index] = 0xff;
  }

  static lv_draw_buf_t draw_buf;
  lv_draw_buf_init(&draw_buf, Width, Height, LV_COLOR_FORMAT_ARGB8888, Stride, mLogoData, DataSize);
  
  static lv_image_dsc_t img_dsc;
  lv_draw_buf_to_image(&draw_buf, &img_dsc);

  lv_obj_t * img1 = lv_image_create(lv_screen_active());
  lv_image_set_src(img1, &img_dsc);
  lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);

}

VOID
LvglUefiDemo (
  VOID
  )
{
  EFI_STATUS                             Status;
  EDKII_PLATFORM_LOGO_PROTOCOL           *PlatformLogo;
  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE  Attribute;
  INTN                                   OffsetX;
  INTN                                   OffsetY;
  UINT32                                 Instance;
  EFI_IMAGE_INPUT                        Image;

  Instance = 0;

  Status = gBS->LocateProtocol (&gEdkiiPlatformLogoProtocolGuid, NULL, (VOID **)&PlatformLogo);
  if (!EFI_ERROR (Status)) {
    //
    // Get EfiImageLogo from PlatformLogo protocol.
    //
    Status = PlatformLogo->GetImage (
                             PlatformLogo,
                             &Instance,
                             &Image,
                             &Attribute,
                             &OffsetX,
                             &OffsetY
                             );
  }

  DisplayUefiLogo(&Image);

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
    Status = UefiLvglAppRegister (LvglUefiDemo);

    if (!EFI_ERROR (Status)) {
      UefiLvglDeinit();
      if (mLogoData != NULL) {
        FreePool (mLogoData);
      }
    }
  }

  return EFI_SUCCESS;
}
