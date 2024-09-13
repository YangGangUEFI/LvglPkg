/**
 * @file lv_port_indev.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"

#include "lvgl/src/indev/lv_indev_private.h"

/*********************
 *      DEFINES
 *********************/
BOOLEAN  mEscExit = FALSE;

extern const lv_img_dsc_t mouse_cursor_icon;

typedef struct {
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsPointer;
  INTN                           LastCursorX;
  INTN                           LastCursorY;
  UINT32                         ActiveButtons;
  BOOLEAN                        LeftButton;
  BOOLEAN                        RightButton;
} LVGL_UEFI_MOUSE;

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void mouse_read(lv_indev_t * indev, lv_indev_data_t * data);

static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);


/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad;

LVGL_UEFI_MOUSE *mLvglUefiMouse = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  ReadKey;

  Status =  gST->ConIn->ReadKeyStroke (gST->ConIn, &ReadKey);
  if (!EFI_ERROR (Status)) {
    switch (ReadKey.UnicodeChar) {
      case CHAR_CARRIAGE_RETURN:
        data->key = LV_KEY_ENTER;
        break;

      case CHAR_BACKSPACE:
        data->key = LV_KEY_BACKSPACE;
        break;

      case CHAR_NULL:
        switch (ReadKey.ScanCode) {
        case SCAN_UP:
          data->key = LV_KEY_UP;
          break;
        
        case SCAN_DOWN:
          data->key = LV_KEY_DOWN;
          break;

        case SCAN_RIGHT:
          data->key = LV_KEY_RIGHT;
          break;

        case SCAN_LEFT:
          data->key = LV_KEY_LEFT;
          break;

        case SCAN_ESC:
          data->key = LV_KEY_ESC;
          mEscExit = TRUE;
          break;

        case SCAN_DELETE:
          data->key = LV_KEY_DEL;
          break;

        case SCAN_PAGE_DOWN:
          data->key = LV_KEY_NEXT;
          break;

        case SCAN_PAGE_UP:
          data->key = LV_KEY_PREV;
          break;

        case SCAN_HOME:
          data->key = LV_KEY_HOME;
          break;

        case SCAN_END:
          data->key = LV_KEY_END;
          break;

        default:
          break;
        }
        break;

      case CHAR_LINEFEED:
      case CHAR_TAB:
        break;

      default:
        data->key = ReadKey.UnicodeChar;
        break;
    }

    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

}


lv_indev_t * lv_uefi_keyboard_create(void)
{
    lv_indev_t * indev = lv_indev_create();
    LV_ASSERT_MALLOC(indev);
    if(indev == NULL) {
        return NULL;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, keypad_read);

    return indev;
}


EFI_STATUS
EFIAPI
GetXY (
  lv_indev_t * indev_drv
  )
{
  EFI_STATUS                     Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsPointer = NULL;
  EFI_ABSOLUTE_POINTER_STATE     AbsState;
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer = NULL;
  EFI_SIMPLE_POINTER_STATE       SimpleState;

  lv_display_t *disp = lv_indev_get_display(indev_drv);

  int32_t hor_res = lv_display_get_horizontal_resolution(disp);
  int32_t ver_res = lv_display_get_vertical_resolution(disp);

  if (mLvglUefiMouse->AbsPointer != NULL) {
    AbsPointer = mLvglUefiMouse->AbsPointer;
    Status = gBS->CheckEvent (AbsPointer->WaitForInput);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_READY;
    }
    Status = AbsPointer->GetState (AbsPointer, &AbsState);
    if (!EFI_ERROR (Status)) {
      mLvglUefiMouse->LastCursorX = (AbsState.CurrentX * hor_res) / (AbsPointer->Mode->AbsoluteMaxX - AbsPointer->Mode->AbsoluteMinX);
      if (mLvglUefiMouse->LastCursorX >= hor_res) {
        mLvglUefiMouse->LastCursorX = hor_res;
      }
      mLvglUefiMouse->LastCursorY = (AbsState.CurrentY * ver_res) / (AbsPointer->Mode->AbsoluteMaxY - AbsPointer->Mode->AbsoluteMinY);
      if (mLvglUefiMouse->LastCursorY >= ver_res) {
        mLvglUefiMouse->LastCursorY = ver_res;
      }
      mLvglUefiMouse->LeftButton = AbsState.ActiveButtons & BIT0;

      return EFI_SUCCESS;
    }
  } else if (mLvglUefiMouse->SimplePointer != NULL) {
    SimplePointer = mLvglUefiMouse->SimplePointer;
    Status = gBS->CheckEvent (SimplePointer->WaitForInput);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_READY;
    }
    Status = SimplePointer->GetState (SimplePointer, &SimpleState);
    if (!EFI_ERROR (Status)) {
      mLvglUefiMouse->LastCursorX += SimpleState.RelativeMovementX / (INT32)SimplePointer->Mode->ResolutionX;
      if (mLvglUefiMouse->LastCursorX >= hor_res) {
        mLvglUefiMouse->LastCursorX = hor_res;
      }
      if (mLvglUefiMouse->LastCursorX <= 0) {
        mLvglUefiMouse->LastCursorX = 0;
      }
      mLvglUefiMouse->LastCursorY += SimpleState.RelativeMovementY / (INT32)SimplePointer->Mode->ResolutionY;
      if (mLvglUefiMouse->LastCursorY >= ver_res) {
        mLvglUefiMouse->LastCursorY = ver_res;
      }
      if (mLvglUefiMouse->LastCursorY <= 0) {
        mLvglUefiMouse->LastCursorY = 0;
      }

      mLvglUefiMouse->LeftButton = SimpleState.LeftButton;
      mLvglUefiMouse->RightButton = SimpleState.RightButton;

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_READY;
}


EFI_STATUS
EFIAPI
EfiMouseInit (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_ABSOLUTE_POINTER_PROTOCOL  *AbsPointer = NULL;
  EFI_SIMPLE_POINTER_PROTOCOL    *SimplePointer = NULL;
  EFI_HANDLE                     *HandleBuffer = NULL;
  UINTN                          HandleCount, Index;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;
  BOOLEAN                        AbsPointerSupport = FALSE;
  BOOLEAN                        SimplePointerSupport = FALSE;

  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimplePointerProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
    if (!EFI_ERROR (Status)) {
      SimplePointerSupport = TRUE;
      break;
    }
  }
  if (HandleBuffer!= NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  HandleCount = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiAbsolutePointerProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePath);
    if (!EFI_ERROR (Status)) {
      AbsPointerSupport = TRUE;
      break;
    }
  }
  if (HandleBuffer!= NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  if (!AbsPointerSupport && !SimplePointerSupport) {
    return EFI_UNSUPPORTED;
  }

  mLvglUefiMouse = malloc (sizeof (LVGL_UEFI_MOUSE));

  DebugPrint (DEBUG_INFO, "EfiMouseInit()\n");

  DebugPrint (DEBUG_INFO, "Abs Pointer: %d\n", AbsPointerSupport);
  DebugPrint (DEBUG_INFO, "Simple Pointer: %d\n", SimplePointerSupport);

  mLvglUefiMouse->AbsPointer = NULL;
  if (AbsPointerSupport) {
    Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiAbsolutePointerProtocolGuid, (VOID **)&AbsPointer);
    if (!EFI_ERROR (Status) && AbsPointer != NULL) {
      AbsPointer->Reset(AbsPointer, TRUE);
      mLvglUefiMouse->AbsPointer = AbsPointer;
    }
  }

  mLvglUefiMouse->SimplePointer = NULL;
  if (SimplePointerSupport && !AbsPointerSupport) {
    Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiSimplePointerProtocolGuid, (VOID **)&SimplePointer);
    if (!EFI_ERROR (Status) && SimplePointer != NULL) {
      SimplePointer->Reset(SimplePointer, TRUE);
      mLvglUefiMouse->SimplePointer = SimplePointer;
    }
  }

  mLvglUefiMouse->ActiveButtons = 0;
  mLvglUefiMouse->LeftButton = FALSE;
  mLvglUefiMouse->RightButton = FALSE;

  return Status;
}


static void mouse_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
  EFI_STATUS   Status;

  Status = GetXY(indev_drv);
  if (!EFI_ERROR (Status)) {
    data->point.x = mLvglUefiMouse->LastCursorX;
    data->point.y = mLvglUefiMouse->LastCursorY;
    if (mLvglUefiMouse->LeftButton) {
      data->state = LV_INDEV_STATE_PRESSED;
      mLvglUefiMouse->LeftButton = FALSE;
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }

}


void lv_indev_set_cusor_start(lv_indev_t * indev)
{
    if(indev == NULL) return;

    lv_display_t *disp = lv_indev_get_display(indev);

    int32_t hor_res = lv_display_get_horizontal_resolution(disp);
    int32_t ver_res = lv_display_get_vertical_resolution(disp);

    indev->pointer.act_point.x = hor_res / 2;
    indev->pointer.act_point.y = ver_res / 2;
    mLvglUefiMouse->LastCursorX = hor_res / 2;
    mLvglUefiMouse->LastCursorY = ver_res / 2;
    mLvglUefiMouse->ActiveButtons = 0;
    mLvglUefiMouse->LeftButton = FALSE;
}

lv_indev_t * lv_uefi_mouse_create(lv_display_t * disp)
{
    lv_indev_t * indev = lv_indev_create();
    LV_ASSERT_MALLOC(indev);
    if(indev == NULL) {
      return NULL;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, mouse_read);
    lv_indev_set_display (indev, disp);

    LV_IMG_DECLARE(mouse_cursor_icon);
    lv_obj_t * mouse_cursor = lv_image_create(lv_screen_active());
    lv_image_set_src(mouse_cursor, &mouse_cursor_icon);
    // lv_indev_set_cusor_start(indev);
    lv_indev_set_cursor(indev, mouse_cursor);

    return indev;
}



void lv_port_indev_init(lv_display_t * disp)
{

    /*------------------
     * Mouse
     * -----------------*/

    /*Initialize your mouse if you have*/
    if (EfiMouseInit() == EFI_SUCCESS) {
      DebugPrint (DEBUG_INFO, "Create Mouse\n");
      lv_uefi_mouse_create(disp);
    }


    /*------------------
     * Keypad
     * -----------------*/
    lv_uefi_keyboard_create();

}

void lv_port_indev_close()
{

    if (mLvglUefiMouse != NULL) {
      free (mLvglUefiMouse);
    }
}

