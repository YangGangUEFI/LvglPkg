#include "UefiSetupCommon.h"


LV_ATTRIBUTE_MEM_ALIGN UINT8 *mLogoData = NULL;


static void uefilogo_delete_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DELETE && mLogoData != NULL) {
        FreePool (mLogoData);
        mLogoData = NULL;
    }
}

lv_obj_t *
DisplayUefiLogo (
  EFI_IMAGE_INPUT *EfiImageLogo,
  lv_obj_t        *parent
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

  lv_obj_t * img1 = lv_image_create(parent);
  lv_obj_add_event_cb(img1, uefilogo_delete_event_cb, LV_EVENT_DELETE, NULL);
  lv_image_set_src(img1, &img_dsc);

  return img1;
}

lv_obj_t *
UefiLogoInfo (
  lv_obj_t * parent
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

  return DisplayUefiLogo(&Image, parent);
}


/**
  Return SMBIOS string for the given string number.

  @param[in] Smbios         Pointer to SMBIOS structure.
  @param[in] StringNumber   String number to return. -1 is used to skip all strings and
                            point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == -1
**/
STATIC
CHAR8 *
LibGetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER  *Smbios,
  IN  UINT16                    StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;

  ASSERT (Smbios != NULL);

  //
  // Skip over formatted section
  //
  String = (CHAR8 *)(Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber; Index++) {
    if (StringNumber == Index) {
      return String;
    }

    //
    // Skip string
    //
    for ( ; *String != 0; String++) {
    }

    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Return pointer to next structure in Smbios.
      //  if you pass in a -1 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}


VOID
UefiBiosInfo (
  lv_obj_t                  *parent,
  EFI_SMBIOS_PROTOCOL       *Smbios
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *SmbiosHdr;
  SMBIOS_STRUCTURE_POINTER  SmbiosStructureP;
  CHAR8                     *BiosVendor, *BiosVersion;
  CHAR8                     BiosInfoStr[256+1];
  int32_t                   obj_w;

  lv_obj_t * BiosInfoLabelT = lv_label_create(parent);
  lv_label_set_text(BiosInfoLabelT, "\nBIOS Info:");
  lv_obj_set_style_text_font(BiosInfoLabelT, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(BiosInfoLabelT, LV_TEXT_ALIGN_LEFT, 0);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_BIOS_INFORMATION;
  Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
  ASSERT(!EFI_ERROR(Status));
  SmbiosStructureP.Hdr = SmbiosHdr;
  BiosVendor = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type0->Vendor);
  BiosVersion = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type0->BiosVersion);

  AsciiSPrint(BiosInfoStr, sizeof (BiosInfoStr), "    Vendor: %a\n    Version: %a", BiosVendor, BiosVersion);
  lv_obj_t * BiosInfoLabel = lv_label_create(parent);
  lv_label_set_text(BiosInfoLabel, BiosInfoStr);
  lv_obj_set_style_text_font(BiosInfoLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(BiosInfoLabel, LV_TEXT_ALIGN_LEFT, 0);

  obj_w = 8 * (int32_t) MAX((AsciiStrSize (BiosVendor) + AsciiStrSize ("    Vendor: ")), (AsciiStrSize (BiosVersion) + AsciiStrSize ("    Version: ")));
  lv_obj_set_width(BiosInfoLabel, obj_w);
  
}


VOID
UefiCpuInfo (
  lv_obj_t                  *parent,
  EFI_SMBIOS_PROTOCOL       *Smbios
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *SmbiosHdr;
  SMBIOS_STRUCTURE_POINTER  SmbiosStructureP;
  UINT8                     SocketCount = 0;
  UINT16                    CpuSpeed = 0;
  CHAR8                     *Manufacturer = NULL, *Version = NULL;
  CHAR8                     CpuInfoStr[256+1];

  lv_obj_t * CpuInfoLabelT = lv_label_create(parent);
  lv_label_set_text(CpuInfoLabelT, "\nCPU Info:");
  lv_obj_set_style_text_font(CpuInfoLabelT, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(CpuInfoLabelT, LV_TEXT_ALIGN_LEFT, 0);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
  while (TRUE) {
    Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }
    SmbiosStructureP.Hdr = SmbiosHdr;

    if (SmbiosStructureP.Type4->CoreCount == 0) {
      continue;
    }

    if (Manufacturer == NULL) {
      Manufacturer = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type4->ProcessorManufacturer);
    }
    if (Version == NULL) {
      Version = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type4->ProcessorVersion);
    }
    if (CpuSpeed == 0) {
      CpuSpeed = SmbiosStructureP.Type4->CurrentSpeed;
    }
    SocketCount++;
  }

  AsciiSPrint(CpuInfoStr, sizeof (CpuInfoStr), "    %a - %a @ %dMHz * %d", Manufacturer, Version, CpuSpeed, SocketCount);
  lv_obj_t * CpuInfoLabel = lv_label_create(parent);
  lv_label_set_text(CpuInfoLabel, CpuInfoStr);
  lv_obj_set_style_text_font(CpuInfoLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(CpuInfoLabel, LV_TEXT_ALIGN_LEFT, 0);
}


VOID
UefiDimmInfo (
  lv_obj_t                  *parent,
  EFI_SMBIOS_PROTOCOL       *Smbios
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *SmbiosHdr;
  SMBIOS_STRUCTURE_POINTER  SmbiosStructureP;
  UINT8                     DimmCount = 0;
  UINT16                    DimmSpeed = 0;
  UINT16                    DimmSize = 0;
  UINT32                    ExtendedSize = 0;
  CHAR8                     *Manufacturer = NULL, *SerialNumber = NULL;
  CHAR8                     DimmInfoStr[256+1];
  CHAR8                     DimmSizeStr[10];

  lv_obj_t * DimmInfoLabelT = lv_label_create(parent);
  lv_label_set_text(DimmInfoLabelT, "\nDIMM Info:");
  lv_obj_set_style_text_font(DimmInfoLabelT, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(DimmInfoLabelT, LV_TEXT_ALIGN_LEFT, 0);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
  while (TRUE) {
    Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, &SmbiosHdr, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }
    SmbiosStructureP.Hdr = SmbiosHdr;

    if (SmbiosStructureP.Type17->Size == 0) {
      continue;
    }

    if (Manufacturer == NULL) {
      Manufacturer = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type17->Manufacturer);
    }
    if (SerialNumber == NULL) {
      SerialNumber = LibGetSmbiosString(&SmbiosStructureP, SmbiosStructureP.Type17->SerialNumber);
    }
    if (DimmSpeed == 0) {
      DimmSpeed = SmbiosStructureP.Type17->ConfiguredMemoryClockSpeed;
    }
    if (DimmSize == 0) {
      DimmSize = SmbiosStructureP.Type17->Size;
    }
    if (ExtendedSize == 0) {
      ExtendedSize = SmbiosStructureP.Type17->ExtendedSize;
    }
    DimmCount++;
  }

  ZeroMem (DimmSizeStr, sizeof (DimmSizeStr));
  if (DimmSize == 0xFFFF || DimmSize == 0x0) {
    DimmSizeStr[0] = '\0';
  } else if (DimmSize == 0x7FFF) {
    AsciiSPrint (DimmSizeStr, sizeof (DimmSizeStr), "%dMB ", (ExtendedSize & (~BIT31)));
  } else {
    if (DimmSize & BIT15) {
      DimmSize = (DimmSize&(~BIT15)) >> 10;
    }
    AsciiSPrint (DimmSizeStr, sizeof (DimmSizeStr), "%dMB ", DimmSize);
  }

  AsciiSPrint(DimmInfoStr, sizeof (DimmInfoStr), "    %a%a(%a) @ %dMT/s * %d", DimmSizeStr, Manufacturer, SerialNumber != NULL ? SerialNumber : "Null SN", DimmSpeed, DimmCount);
  lv_obj_t * DimmInfoLabel = lv_label_create(parent);
  lv_label_set_text(DimmInfoLabel, DimmInfoStr);
  lv_obj_set_style_text_font(DimmInfoLabel, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_align(DimmInfoLabel, LV_TEXT_ALIGN_LEFT, 0);
}


VOID
UefiBasicInfo (
  lv_obj_t * parent
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;

  Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  if (!EFI_ERROR (Status)) {
    UefiLogoInfo (parent);
    UefiBiosInfo (parent, Smbios);
    UefiCpuInfo (parent, Smbios);
    UefiDimmInfo (parent, Smbios);
  }
}
