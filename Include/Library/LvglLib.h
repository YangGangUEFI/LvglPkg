/** @file
  LvglLib class with APIs from the openssl project

  Copyright (c) 2024, Yang Gang. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LVGL_LIB_H__
#define __LVGL_LIB_H__

#include <lvgl.h>

typedef
VOID
(EFIAPI *EFI_LVGL_APP_FUNCTION)(
  VOID
  );

EFI_STATUS
EFIAPI
UefiLvglInit (
  VOID
  );

EFI_STATUS
EFIAPI
UefiLvglDeinit (
  VOID
  );

EFI_STATUS
EFIAPI
UefiLvglAppRegister (
  IN EFI_LVGL_APP_FUNCTION AppRegister
  );

#endif