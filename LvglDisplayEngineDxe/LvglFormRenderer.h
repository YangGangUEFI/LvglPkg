/** @file
  LVGL Form Renderer — builds LVGL widgets from FORM_DISPLAY_ENGINE_FORM.

  Copyright (c) 2024-2026, Hamit Karaca. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LVGL_FORM_RENDERER_H_
#define LVGL_FORM_RENDERER_H_

#include <Uefi.h>
#include <Library/LvglLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DisplayProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

//
// Per-widget user data linking an LVGL object back to its statement.
//
typedef struct {
  FORM_DISPLAY_ENGINE_STATEMENT    *Statement;
  lv_obj_t                        *Widget;
} LVGL_STATEMENT_CONTEXT;

//
// Session state for a single FormDisplay() call.
//
typedef struct {
  FORM_DISPLAY_ENGINE_FORM    *FormData;
  USER_INPUT                  *UserInput;
  //
  // Set to TRUE by an event handler when the user has made a selection
  // or pressed ESC. The main loop checks this to break.
  //
  BOOLEAN                     ExitRequested;
  //
  // LVGL screen object for this form. Deleted on exit.
  //
  lv_obj_t                    *Screen;
  //
  // Focused group for keyboard navigation.
  //
  lv_group_t                  *Group;
} LVGL_FORM_SESSION;

/**
  Build LVGL widgets for a form and run the event loop until
  the user performs an action.

  @param[in]  FormData       Parsed form from SetupBrowserDxe.
  @param[out] UserInputData  Filled with the user's selection/action.

  @retval EFI_SUCCESS        User input captured.
**/
EFI_STATUS
EFIAPI
LvglRenderForm (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT                *UserInputData
  );

/**
  Tear down LVGL objects created by the renderer.
**/
VOID
EFIAPI
LvglRendererCleanup (
  VOID
  );

#endif // LVGL_FORM_RENDERER_H_
