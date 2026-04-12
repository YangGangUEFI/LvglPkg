/** @file
  LVGL-based Display Engine DXE driver.

  Produces EDKII_FORM_DISPLAY_ENGINE_PROTOCOL so that SetupBrowserDxe can
  call FormDisplay(), ExitDisplay(), and ConfirmDataChange().  FormDisplay()
  delegates to LvglFormRenderer which builds LVGL widgets from the
  FORM_DISPLAY_ENGINE_FORM structure and runs the LVGL event loop.

  Copyright (c) 2024-2026, Hamit Karaca. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/DisplayProtocol.h>
#include "LvglFormRenderer.h"

//
// Private data
//
#define LVGL_DISPLAY_ENGINE_SIGNATURE  SIGNATURE_32 ('L', 'D', 'E', 'N')

typedef struct {
  UINT32                                Signature;
  EFI_HANDLE                            Handle;
  EDKII_FORM_DISPLAY_ENGINE_PROTOCOL    Protocol;
} LVGL_DISPLAY_ENGINE_PRIVATE_DATA;

STATIC LVGL_DISPLAY_ENGINE_PRIVATE_DATA  mPrivateData;

/**
  Display one form and return user input.

  @param[in]  FormData        Form data to be displayed.
  @param[out] UserInputData   User input result.

  @retval EFI_SUCCESS         Form displayed and user input captured.
**/
STATIC
EFI_STATUS
EFIAPI
LvglFormDisplay (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT                *UserInputData
  )
{
  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: FormDisplay() called — FormId=0x%x\n", FormData->FormId));

  return LvglRenderForm (FormData, UserInputData);
}

/**
  Exit display and clean up.
**/
STATIC
VOID
EFIAPI
LvglExitDisplay (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: ExitDisplay() called\n"));

  LvglRendererCleanup ();
}

/**
  Confirm how to handle changed data (submit / discard / none).

  @return Action — BROWSER_ACTION_SUBMIT, BROWSER_ACTION_DISCARD, or BROWSER_ACTION_NONE.
**/
STATIC
UINTN
EFIAPI
LvglConfirmDataChange (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: ConfirmDataChange() called\n"));

  //
  // TODO: Show LVGL dialog asking user to save/discard/cancel.
  //

  //
  // Default: discard unsaved changes.
  //
  return BROWSER_ACTION_NONE;
}

/**
  Entry point — install EDKII_FORM_DISPLAY_ENGINE_PROTOCOL.

  @param[in] ImageHandle   Driver image handle.
  @param[in] SystemTable   Pointer to EFI System Table.

  @retval EFI_SUCCESS      Protocol installed successfully.
**/
EFI_STATUS
EFIAPI
LvglDisplayEngineInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: initializing\n"));

  mPrivateData.Signature             = LVGL_DISPLAY_ENGINE_SIGNATURE;
  mPrivateData.Handle                = NULL;
  mPrivateData.Protocol.FormDisplay      = LvglFormDisplay;
  mPrivateData.Protocol.ExitDisplay      = LvglExitDisplay;
  mPrivateData.Protocol.ConfirmDataChange = LvglConfirmDataChange;

  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEdkiiFormDisplayEngineProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.Protocol
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: protocol installed — %r\n", Status));

  return Status;
}

/**
  Unload handler — uninstall the protocol.

  @param[in] ImageHandle   Driver image handle.

  @retval EFI_SUCCESS      Protocol uninstalled.
**/
EFI_STATUS
EFIAPI
LvglDisplayEngineUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  mPrivateData.Handle,
                  &gEdkiiFormDisplayEngineProtocolGuid,
                  &mPrivateData.Protocol
                  );

  DEBUG ((DEBUG_INFO, "LvglDisplayEngine: unloaded — %r\n", Status));

  return Status;
}
