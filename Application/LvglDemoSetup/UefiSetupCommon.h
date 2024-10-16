#ifndef __UEFI_SETUP_COMMON_H__
#define __UEFI_SETUP_COMMON_H__

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/PlatformLogo.h>
#include <Protocol/Smbios.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

#include <Library/LvglLib.h>


VOID
UefiBasicInfo (
  lv_obj_t * parent
  );

VOID
UefiBootOptions (
  lv_obj_t * parent
  );

#endif
