/** @file
  LVGL Form Renderer — creates LVGL widgets from FORM_DISPLAY_ENGINE_FORM.

  Walks the StatementListHead provided by SetupBrowserDxe and maps each
  IFR opcode to a corresponding LVGL widget. Runs the LVGL event loop
  until the user selects a question or presses ESC, then fills USER_INPUT
  for the browser.

  Copyright (c) 2024-2026, Hamit Karaca. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LvglFormRenderer.h"

STATIC LVGL_FORM_SESSION  mSession;
STATIC BOOLEAN            mLvglReady = FALSE;

//
// Forward declarations for widget builders.
//
STATIC VOID CreateSubtitleWidget  (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle);
STATIC VOID CreateTextWidget      (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle);
STATIC VOID CreateCheckboxWidget  (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);
STATIC VOID CreateNumericWidget   (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);
STATIC VOID CreateOneOfWidget     (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);
STATIC VOID CreateStringWidget    (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);
STATIC VOID CreateRefWidget       (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);
STATIC VOID CreateActionWidget    (lv_obj_t *Parent, FORM_DISPLAY_ENGINE_STATEMENT *Statement, EFI_HII_HANDLE HiiHandle, lv_group_t *Group);

//
// ---- Helper: UCS-2 string to UTF-8 for LVGL ----
//

/**
  Convert a UCS-2 (CHAR16) string to a UTF-8 (CHAR8) buffer.
  Caller must FreePool() the result.

  @param[in] Str16  UCS-2 string.

  @return Allocated UTF-8 string, or NULL on failure.
**/
STATIC
CHAR8 *
Ucs2ToUtf8 (
  IN CONST CHAR16  *Str16
  )
{
  UINTN  Len;
  UINTN  Idx;
  UINTN  Out;
  CHAR8  *Utf8;

  if (Str16 == NULL) {
    return NULL;
  }

  //
  // Worst case: each UCS-2 char becomes 3 UTF-8 bytes.
  //
  for (Len = 0; Str16[Len] != 0; Len++) {
  }

  Utf8 = AllocatePool (Len * 3 + 1);
  if (Utf8 == NULL) {
    return NULL;
  }

  Out = 0;
  for (Idx = 0; Idx < Len; Idx++) {
    UINT16  Ch = Str16[Idx];

    if (Ch < 0x80) {
      Utf8[Out++] = (CHAR8)Ch;
    } else if (Ch < 0x800) {
      Utf8[Out++] = (CHAR8)(0xC0 | (Ch >> 6));
      Utf8[Out++] = (CHAR8)(0x80 | (Ch & 0x3F));
    } else {
      Utf8[Out++] = (CHAR8)(0xE0 | (Ch >> 12));
      Utf8[Out++] = (CHAR8)(0x80 | ((Ch >> 6) & 0x3F));
      Utf8[Out++] = (CHAR8)(0x80 | (Ch & 0x3F));
    }
  }

  Utf8[Out] = '\0';
  return Utf8;
}

/**
  Get prompt text for a statement as UTF-8. Caller must FreePool().
**/
STATIC
CHAR8 *
GetPromptUtf8 (
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN EFI_HII_HANDLE                HiiHandle
  )
{
  EFI_STRING_ID  PromptId;
  CHAR16         *Str16;
  CHAR8          *Utf8;

  //
  // The Prompt field is at the same offset in both EFI_IFR_STATEMENT_HEADER
  // and EFI_IFR_QUESTION_HEADER (which embeds STATEMENT_HEADER first).
  // Cast through the smallest common structure.
  //
  PromptId = ((EFI_IFR_SUBTITLE *)Statement->OpCode)->Statement.Prompt;
  if (PromptId == 0) {
    return NULL;
  }

  Str16 = HiiGetString (HiiHandle, PromptId, NULL);
  if (Str16 == NULL) {
    return NULL;
  }

  Utf8 = Ucs2ToUtf8 (Str16);
  FreePool (Str16);
  return Utf8;
}

//
// ---- Event callbacks ----
//

/**
  Generic click handler — records the statement selection and requests exit.
**/
STATIC
VOID
OnStatementClicked (
  lv_event_t  *Event
  )
{
  LVGL_STATEMENT_CONTEXT  *Ctx;

  Ctx = (LVGL_STATEMENT_CONTEXT *)lv_event_get_user_data (Event);
  if (Ctx == NULL || mSession.UserInput == NULL) {
    return;
  }

  mSession.UserInput->SelectedStatement = Ctx->Statement;
  CopyMem (
    &mSession.UserInput->InputValue,
    &Ctx->Statement->CurrentValue,
    sizeof (EFI_HII_VALUE)
    );
  mSession.UserInput->Action = 0;
  mSession.ExitRequested     = TRUE;
}

/**
  Checkbox value-changed handler — toggles the boolean and records it.
**/
STATIC
VOID
OnCheckboxChanged (
  lv_event_t  *Event
  )
{
  LVGL_STATEMENT_CONTEXT  *Ctx;
  lv_obj_t                *Cb;

  Ctx = (LVGL_STATEMENT_CONTEXT *)lv_event_get_user_data (Event);
  if (Ctx == NULL || mSession.UserInput == NULL) {
    return;
  }

  Cb = lv_event_get_target_obj (Event);

  mSession.UserInput->SelectedStatement          = Ctx->Statement;
  mSession.UserInput->InputValue.Type            = EFI_IFR_TYPE_BOOLEAN;
  mSession.UserInput->InputValue.Value.b         = lv_obj_has_state (Cb, LV_STATE_CHECKED) ? TRUE : FALSE;
  mSession.UserInput->Action                     = 0;
  mSession.ExitRequested                         = TRUE;
}

/**
  Dropdown value-changed handler — records the selected option index.
**/
STATIC
VOID
OnDropdownChanged (
  lv_event_t  *Event
  )
{
  LVGL_STATEMENT_CONTEXT       *Ctx;
  lv_obj_t                     *Dd;
  UINT32                       SelIdx;
  LIST_ENTRY                   *Link;
  DISPLAY_QUESTION_OPTION      *Option;
  UINT32                       CurIdx;

  Ctx = (LVGL_STATEMENT_CONTEXT *)lv_event_get_user_data (Event);
  if (Ctx == NULL || mSession.UserInput == NULL) {
    return;
  }

  Dd     = lv_event_get_target_obj (Event);
  SelIdx = lv_dropdown_get_selected (Dd);

  //
  // Walk the option list to find the selected option's value.
  //
  CurIdx = 0;
  for (Link = Ctx->Statement->OptionListHead.ForwardLink;
       Link != &Ctx->Statement->OptionListHead;
       Link = Link->ForwardLink)
  {
    Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    if (CurIdx == SelIdx) {
      mSession.UserInput->SelectedStatement = Ctx->Statement;
      mSession.UserInput->InputValue.Type   = Option->OptionOpCode->Type;
      CopyMem (
        &mSession.UserInput->InputValue.Value,
        &Option->OptionOpCode->Value,
        sizeof (EFI_IFR_TYPE_VALUE)
        );
      mSession.UserInput->Action = 0;
      mSession.ExitRequested     = TRUE;
      return;
    }

    CurIdx++;
  }
}

/**
  ESC key handler — signals form exit.
**/
STATIC
VOID
OnEscPressed (
  lv_event_t  *Event
  )
{
  lv_key_t  Key;

  Key = lv_indev_get_key (lv_indev_active ());
  if (Key == LV_KEY_ESC) {
    mSession.UserInput->Action    = BROWSER_ACTION_FORM_EXIT;
    mSession.UserInput->SelectedStatement = NULL;
    mSession.ExitRequested        = TRUE;
  }
}

//
// ---- Widget builders ----
//

STATIC
VOID
CreateSubtitleWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle
  )
{
  CHAR8     *Text;
  lv_obj_t  *Label;

  Text = GetPromptUtf8 (Statement, HiiHandle);
  if (Text == NULL) {
    return;
  }

  Label = lv_label_create (Parent);
  lv_label_set_text (Label, Text);
  lv_obj_set_style_text_font (Label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color (Label, lv_palette_main (LV_PALETTE_BLUE), 0);
  lv_obj_set_style_pad_top (Label, 8, 0);

  FreePool (Text);
}

STATIC
VOID
CreateTextWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle
  )
{
  CHAR8     *Text;
  lv_obj_t  *Label;

  Text = GetPromptUtf8 (Statement, HiiHandle);
  if (Text == NULL) {
    return;
  }

  Label = lv_label_create (Parent);
  lv_label_set_text (Label, Text);

  FreePool (Text);
}

STATIC
VOID
CreateCheckboxWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  CHAR8                   *Text;
  lv_obj_t                *Cb;
  LVGL_STATEMENT_CONTEXT  *Ctx;

  Text = GetPromptUtf8 (Statement, HiiHandle);

  Cb = lv_checkbox_create (Parent);
  lv_checkbox_set_text (Cb, Text != NULL ? Text : "Checkbox");

  if (Statement->CurrentValue.Value.b) {
    lv_obj_add_state (Cb, LV_STATE_CHECKED);
  }

  if (Statement->Attribute & HII_DISPLAY_GRAYOUT) {
    lv_obj_add_state (Cb, LV_STATE_DISABLED);
  }

  Ctx = AllocateZeroPool (sizeof (LVGL_STATEMENT_CONTEXT));
  if (Ctx != NULL) {
    Ctx->Statement = Statement;
    Ctx->Widget    = Cb;
    lv_obj_add_event_cb (Cb, OnCheckboxChanged, LV_EVENT_VALUE_CHANGED, Ctx);
  }

  lv_group_add_obj (Group, Cb);

  if (Text != NULL) {
    FreePool (Text);
  }
}

STATIC
VOID
CreateNumericWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  CHAR8                   *Text;
  lv_obj_t                *Row;
  lv_obj_t                *Label;
  lv_obj_t                *Spinbox;
  EFI_IFR_NUMERIC         *NumOp;
  LVGL_STATEMENT_CONTEXT  *Ctx;

  Text = GetPromptUtf8 (Statement, HiiHandle);

  //
  // Create a horizontal row: label + spinbox.
  //
  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, LV_PCT (100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow (Row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all (Row, 4, 0);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);

  Label = lv_label_create (Row);
  lv_label_set_text (Label, Text != NULL ? Text : "Numeric");
  lv_obj_set_flex_grow (Label, 1);

  Spinbox = lv_spinbox_create (Row);

  NumOp = (EFI_IFR_NUMERIC *)Statement->OpCode;
  switch (NumOp->Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      lv_spinbox_set_range (Spinbox, (INT32)NumOp->data.u8.MinValue, (INT32)NumOp->data.u8.MaxValue);
      lv_spinbox_set_value (Spinbox, (INT32)Statement->CurrentValue.Value.u8);
      break;
    case EFI_IFR_NUMERIC_SIZE_2:
      lv_spinbox_set_range (Spinbox, (INT32)NumOp->data.u16.MinValue, (INT32)NumOp->data.u16.MaxValue);
      lv_spinbox_set_value (Spinbox, (INT32)Statement->CurrentValue.Value.u16);
      break;
    case EFI_IFR_NUMERIC_SIZE_4:
      lv_spinbox_set_range (Spinbox, (INT32)NumOp->data.u32.MinValue, (INT32)NumOp->data.u32.MaxValue);
      lv_spinbox_set_value (Spinbox, (INT32)Statement->CurrentValue.Value.u32);
      break;
    default:
      lv_spinbox_set_range (Spinbox, 0, 0x7FFFFFFF);
      lv_spinbox_set_value (Spinbox, (INT32)Statement->CurrentValue.Value.u64);
      break;
  }

  if (Statement->Attribute & HII_DISPLAY_GRAYOUT) {
    lv_obj_add_state (Spinbox, LV_STATE_DISABLED);
  }

  Ctx = AllocateZeroPool (sizeof (LVGL_STATEMENT_CONTEXT));
  if (Ctx != NULL) {
    Ctx->Statement = Statement;
    Ctx->Widget    = Spinbox;
    lv_obj_add_event_cb (Spinbox, OnStatementClicked, LV_EVENT_VALUE_CHANGED, Ctx);
  }

  lv_group_add_obj (Group, Spinbox);

  if (Text != NULL) {
    FreePool (Text);
  }
}

STATIC
VOID
CreateOneOfWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  CHAR8                     *Text;
  lv_obj_t                  *Row;
  lv_obj_t                  *Label;
  lv_obj_t                  *Dd;
  LIST_ENTRY                *Link;
  DISPLAY_QUESTION_OPTION   *Option;
  CHAR8                     OptBuf[512];
  UINTN                     OptLen;
  CHAR16                    *OptStr16;
  CHAR8                     *OptStr8;
  UINT32                    SelectedIdx;
  UINT32                    CurIdx;
  LVGL_STATEMENT_CONTEXT    *Ctx;

  Text = GetPromptUtf8 (Statement, HiiHandle);

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, LV_PCT (100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow (Row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all (Row, 4, 0);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);

  Label = lv_label_create (Row);
  lv_label_set_text (Label, Text != NULL ? Text : "OneOf");
  lv_obj_set_flex_grow (Label, 1);

  //
  // Build newline-separated option string for LVGL dropdown.
  //
  OptLen      = 0;
  OptBuf[0]   = '\0';
  SelectedIdx = 0;
  CurIdx      = 0;

  for (Link = Statement->OptionListHead.ForwardLink;
       Link != &Statement->OptionListHead;
       Link = Link->ForwardLink)
  {
    Option   = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    OptStr16 = HiiGetString (HiiHandle, Option->OptionOpCode->Option, NULL);
    if (OptStr16 != NULL) {
      OptStr8 = Ucs2ToUtf8 (OptStr16);
      if (OptStr8 != NULL) {
        UINTN  ItemLen = AsciiStrLen (OptStr8);
        if (OptLen + ItemLen + 2 < sizeof (OptBuf)) {
          if (OptLen > 0) {
            OptBuf[OptLen++] = '\n';
          }

          AsciiStrCpyS (&OptBuf[OptLen], sizeof (OptBuf) - OptLen, OptStr8);
          OptLen += ItemLen;
        }

        FreePool (OptStr8);
      }

      FreePool (OptStr16);
    }

    //
    // Check if this option matches the current value.
    //
    if (CompareMem (&Option->OptionOpCode->Value, &Statement->CurrentValue.Value, sizeof (EFI_IFR_TYPE_VALUE)) == 0) {
      SelectedIdx = CurIdx;
    }

    CurIdx++;
  }

  Dd = lv_dropdown_create (Row);
  lv_dropdown_set_options (Dd, OptBuf);
  lv_dropdown_set_selected (Dd, SelectedIdx);

  if (Statement->Attribute & HII_DISPLAY_GRAYOUT) {
    lv_obj_add_state (Dd, LV_STATE_DISABLED);
  }

  Ctx = AllocateZeroPool (sizeof (LVGL_STATEMENT_CONTEXT));
  if (Ctx != NULL) {
    Ctx->Statement = Statement;
    Ctx->Widget    = Dd;
    lv_obj_add_event_cb (Dd, OnDropdownChanged, LV_EVENT_VALUE_CHANGED, Ctx);
  }

  lv_group_add_obj (Group, Dd);

  if (Text != NULL) {
    FreePool (Text);
  }
}

STATIC
VOID
CreateStringWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  CHAR8                   *Text;
  lv_obj_t                *Row;
  lv_obj_t                *Label;
  lv_obj_t                *Ta;
  LVGL_STATEMENT_CONTEXT  *Ctx;

  Text = GetPromptUtf8 (Statement, HiiHandle);

  Row = lv_obj_create (Parent);
  lv_obj_set_size (Row, LV_PCT (100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow (Row, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all (Row, 4, 0);
  lv_obj_set_style_border_width (Row, 0, 0);
  lv_obj_set_style_bg_opa (Row, LV_OPA_TRANSP, 0);

  Label = lv_label_create (Row);
  lv_label_set_text (Label, Text != NULL ? Text : "String");
  lv_obj_set_flex_grow (Label, 1);

  Ta = lv_textarea_create (Row);
  lv_textarea_set_one_line (Ta, true);

  if (Statement->OpCode->OpCode == EFI_IFR_PASSWORD_OP) {
    lv_textarea_set_password_mode (Ta, true);
  }

  if (Statement->Attribute & HII_DISPLAY_GRAYOUT) {
    lv_obj_add_state (Ta, LV_STATE_DISABLED);
  }

  Ctx = AllocateZeroPool (sizeof (LVGL_STATEMENT_CONTEXT));
  if (Ctx != NULL) {
    Ctx->Statement = Statement;
    Ctx->Widget    = Ta;
    lv_obj_add_event_cb (Ta, OnStatementClicked, LV_EVENT_READY, Ctx);
  }

  lv_group_add_obj (Group, Ta);

  if (Text != NULL) {
    FreePool (Text);
  }
}

STATIC
VOID
CreateRefWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  CHAR8                   *Text;
  lv_obj_t                *Btn;
  lv_obj_t                *Label;
  LVGL_STATEMENT_CONTEXT  *Ctx;

  Text = GetPromptUtf8 (Statement, HiiHandle);

  Btn   = lv_btn_create (Parent);
  Label = lv_label_create (Btn);
  lv_label_set_text (Label, Text != NULL ? Text : "Goto");
  lv_obj_set_width (Btn, LV_PCT (100));

  if (Statement->Attribute & HII_DISPLAY_GRAYOUT) {
    lv_obj_add_state (Btn, LV_STATE_DISABLED);
  }

  Ctx = AllocateZeroPool (sizeof (LVGL_STATEMENT_CONTEXT));
  if (Ctx != NULL) {
    Ctx->Statement = Statement;
    Ctx->Widget    = Btn;
    lv_obj_add_event_cb (Btn, OnStatementClicked, LV_EVENT_CLICKED, Ctx);
  }

  lv_group_add_obj (Group, Btn);

  if (Text != NULL) {
    FreePool (Text);
  }
}

STATIC
VOID
CreateActionWidget (
  lv_obj_t                        *Parent,
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  EFI_HII_HANDLE                 HiiHandle,
  lv_group_t                      *Group
  )
{
  //
  // Action buttons look identical to Ref (goto) buttons from the UI side.
  //
  CreateRefWidget (Parent, Statement, HiiHandle, Group);
}

//
// ---- Core renderer ----
//

/**
  Walk StatementListHead and create one LVGL widget per visible statement.
**/
STATIC
VOID
BuildFormWidgets (
  IN LVGL_FORM_SESSION  *Session
  )
{
  LIST_ENTRY                       *Link;
  FORM_DISPLAY_ENGINE_STATEMENT    *Statement;
  EFI_HII_HANDLE                  HiiHandle;

  HiiHandle = Session->FormData->HiiHandle;

  for (Link = Session->FormData->StatementListHead.ForwardLink;
       Link != &Session->FormData->StatementListHead;
       Link = Link->ForwardLink)
  {
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);

    //
    // Skip suppressed statements.
    //
    if (Statement->Attribute & HII_DISPLAY_SUPPRESS) {
      continue;
    }

    switch (Statement->OpCode->OpCode) {
      case EFI_IFR_SUBTITLE_OP:
        CreateSubtitleWidget (Session->Screen, Statement, HiiHandle);
        break;

      case EFI_IFR_TEXT_OP:
        CreateTextWidget (Session->Screen, Statement, HiiHandle);
        break;

      case EFI_IFR_CHECKBOX_OP:
        CreateCheckboxWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      case EFI_IFR_NUMERIC_OP:
        CreateNumericWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      case EFI_IFR_ONE_OF_OP:
        CreateOneOfWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      case EFI_IFR_STRING_OP:
      case EFI_IFR_PASSWORD_OP:
        CreateStringWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      case EFI_IFR_REF_OP:
        CreateRefWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      case EFI_IFR_ACTION_OP:
        CreateActionWidget (Session->Screen, Statement, HiiHandle, Session->Group);
        break;

      default:
        DEBUG ((DEBUG_VERBOSE, "LvglRenderer: skipping opcode 0x%02x\n", Statement->OpCode->OpCode));
        break;
    }
  }
}

EFI_STATUS
EFIAPI
LvglRenderForm (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT                *UserInputData
  )
{
  EFI_STATUS  Status;
  CHAR16      *TitleStr16;
  CHAR8       *TitleStr8;
  lv_obj_t    *TitleLabel;
  lv_obj_t    *ContentPanel;
  extern BOOLEAN mTickSupport;

  ASSERT (FormData != NULL);
  ASSERT (UserInputData != NULL);

  //
  // Initialize LVGL if not yet done (constructor in LvglLib handles this,
  // but call explicitly to be safe).
  //
  if (!mLvglReady) {
    Status = UefiLvglInit ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "LvglRenderer: UefiLvglInit failed — %r\n", Status));
      return Status;
    }

    mLvglReady = TRUE;
  }

  //
  // Clear the console and hide cursor for graphical mode.
  //
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  //
  // Set up session state.
  //
  ZeroMem (&mSession, sizeof (mSession));
  mSession.FormData       = FormData;
  mSession.UserInput      = UserInputData;
  mSession.ExitRequested  = FALSE;

  ZeroMem (UserInputData, sizeof (USER_INPUT));

  //
  // Create a new screen.
  //
  mSession.Screen = lv_obj_create (NULL);
  lv_obj_set_style_bg_color (mSession.Screen, lv_color_hex (0x1A1A2E), 0);
  lv_obj_set_flex_flow (mSession.Screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all (mSession.Screen, 16, 0);
  lv_obj_set_style_pad_row (mSession.Screen, 6, 0);

  //
  // Add an ESC key handler on the screen.
  //
  lv_obj_add_event_cb (mSession.Screen, OnEscPressed, LV_EVENT_KEY, NULL);

  //
  // Title bar.
  //
  TitleStr16 = HiiGetString (FormData->HiiHandle, FormData->FormTitle, NULL);
  TitleStr8  = Ucs2ToUtf8 (TitleStr16 != NULL ? TitleStr16 : L"Setup");
  if (TitleStr16 != NULL) {
    FreePool (TitleStr16);
  }

  TitleLabel = lv_label_create (mSession.Screen);
  lv_label_set_text (TitleLabel, TitleStr8 != NULL ? TitleStr8 : "Setup");
  lv_obj_set_style_text_font (TitleLabel, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color (TitleLabel, lv_color_white (), 0);
  if (TitleStr8 != NULL) {
    FreePool (TitleStr8);
  }

  //
  // Scrollable content area.
  //
  ContentPanel = lv_obj_create (mSession.Screen);
  lv_obj_set_size (ContentPanel, LV_PCT (100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow (ContentPanel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_grow (ContentPanel, 1);
  lv_obj_set_style_pad_all (ContentPanel, 8, 0);
  lv_obj_set_style_pad_row (ContentPanel, 4, 0);
  lv_obj_set_style_bg_color (ContentPanel, lv_color_hex (0x16213E), 0);
  lv_obj_set_style_radius (ContentPanel, 8, 0);

  //
  // Create a navigation group and bind keyboard input devices.
  //
  mSession.Group = lv_group_create ();
  {
    lv_indev_t  *Indev = NULL;

    for (;;) {
      Indev = lv_indev_get_next (Indev);
      if (Indev == NULL) {
        break;
      }

      if (lv_indev_get_type (Indev) == LV_INDEV_TYPE_KEYPAD) {
        lv_indev_set_group (Indev, mSession.Group);
      }
    }
  }

  //
  // Temporarily swap mSession.Screen to use ContentPanel for widget creation,
  // so widgets go inside the scrollable panel.
  //
  {
    lv_obj_t  *OrigScreen = mSession.Screen;

    mSession.Screen = ContentPanel;
    BuildFormWidgets (&mSession);
    mSession.Screen = OrigScreen;
  }

  //
  // Load the screen.
  //
  lv_screen_load (mSession.Screen);

  //
  // LVGL event loop — run until user makes a selection or presses ESC.
  //
  DEBUG ((DEBUG_INFO, "LvglRenderer: entering event loop for FormId=0x%x\n", FormData->FormId));

  while (!mSession.ExitRequested) {
    lv_timer_handler ();
    gBS->Stall (10 * 1000);  // 10 ms
    if (!mTickSupport) {
      lv_tick_inc (10);
    }
  }

  DEBUG ((DEBUG_INFO, "LvglRenderer: exiting event loop — Action=0x%x\n", UserInputData->Action));

  return EFI_SUCCESS;
}

VOID
EFIAPI
LvglRendererCleanup (
  VOID
  )
{
  if (mSession.Group != NULL) {
    lv_group_delete (mSession.Group);
    mSession.Group = NULL;
  }

  if (mSession.Screen != NULL) {
    lv_obj_delete (mSession.Screen);
    mSession.Screen = NULL;
  }

  //
  // Note: LVGL_STATEMENT_CONTEXT objects were allocated with AllocateZeroPool
  // and are cleaned up when the screen is deleted (LVGL deletes children
  // recursively). However, user_data pointers are NOT freed by LVGL.
  // For a production implementation, we'd track and free them.
  // For now this is acceptable as the browser re-calls FormDisplay() in a loop
  // and the DXE pool is reclaimed on reboot.
  //
}
