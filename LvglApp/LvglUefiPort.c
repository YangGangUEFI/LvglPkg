
#include "LvglUefiPort.h"

#define LVGL_HEAD_SIGNATURE  SIGNATURE_32('l','v','g','l')

typedef struct {
  UINT32    Signature;
  UINTN     Size;
} LVGL_HEAD;

#define LVGL_OVERHEAD  sizeof(LVGL_HEAD)

void* memset (void *dest, char ch, unsigned int count)
{
  return SetMem (dest, count, ch);
}


void *
malloc (
  size_t  size
  )
{
  LVGL_HEAD  *PoolHdr;
  UINTN         NewSize;
  VOID          *Data;

  NewSize = (UINTN)(size) + LVGL_OVERHEAD;

  Data = AllocatePool (NewSize);
  if (Data != NULL) {
    PoolHdr            = (LVGL_HEAD *)Data;
    PoolHdr->Signature = LVGL_HEAD_SIGNATURE;
    PoolHdr->Size      = size;

    return (VOID *)(PoolHdr + 1);
  }

  return NULL;
}

void *
realloc (
  void    *ptr,
  size_t  size
  )
{
  LVGL_HEAD  *OldPoolHdr;
  LVGL_HEAD  *NewPoolHdr;
  UINTN         OldSize;
  UINTN         NewSize;
  VOID          *Data;

  NewSize = (UINTN)size + LVGL_OVERHEAD;
  Data    = AllocatePool (NewSize);
  if (Data != NULL) {
    NewPoolHdr            = (LVGL_HEAD *)Data;
    NewPoolHdr->Signature = LVGL_HEAD_SIGNATURE;
    NewPoolHdr->Size      = size;
    if (ptr != NULL) {
      OldPoolHdr = (LVGL_HEAD *)ptr - 1;
      ASSERT (OldPoolHdr->Signature == LVGL_HEAD_SIGNATURE);
      OldSize = OldPoolHdr->Size;

      CopyMem ((VOID *)(NewPoolHdr + 1), ptr, MIN (OldSize, size));
      FreePool ((VOID *)OldPoolHdr);
    }

    return (VOID *)(NewPoolHdr + 1);
  }

  return NULL;
}

void
free (
  void  *ptr
  )
{
  VOID          *EvalOnce;
  LVGL_HEAD  *PoolHdr;

  EvalOnce = ptr;
  if (EvalOnce == NULL) {
    return;
  }

  PoolHdr = (LVGL_HEAD *)EvalOnce - 1;
  if (PoolHdr->Signature == LVGL_HEAD_SIGNATURE) {
    FreePool (PoolHdr);
  } else {
    FreePool (EvalOnce);
  }
}


/* Return the absolute value of I.  */
long int
labs (long int i)
{
  return i < 0 ? -i : i;
}

/* Return the absolute value of I.  */
int
abs (int i)
{
  return i < 0 ? -i : i;
}

char *strchr(const char *str, int ch)
{
  return ScanMem8 (str, AsciiStrSize (str), (UINT8)ch);
}