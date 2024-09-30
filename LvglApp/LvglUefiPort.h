
#ifndef __LVGL_UEFI_PORT_H__
#define __LVGL_UEFI_PORT_H__

#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>


typedef UINTN   size_t;
typedef UINT32  uint32_t;
typedef UINT16  uint16_t;
typedef UINT8   uint8_t;
typedef INTN    intptr_t;

#ifndef offsetof
#define offsetof  OFFSET_OF
#endif


#define calloc(n, s)                      AllocateZeroPool((n)*(s))
#define memcpy(dest,source,count)         CopyMem(dest,source,(UINTN)(count))
// #define memset(dest,ch,count)             SetMem(dest,(UINTN)(count),(UINT8)(ch))
#define memchr(buf,ch,count)              ScanMem8(buf,(UINTN)(count),(UINT8)ch)
#define memcmp(buf1,buf2,count)           (int)(CompareMem(buf1,buf2,(UINTN)(count)))
#define memmove(dest,source,count)        CopyMem(dest,source,(UINTN)(count))

#define va_init_list(a, b)  VA_START(a,b)
#define va_list  VA_LIST
#define va_arg(a, b)  VA_ARG(a,b)
#define va_end(a)     VA_END(a)
#define va_start  VA_START
#define va_copy  VA_COPY

#define FILE    VOID
#define stdout  NULL
#define fprintf(...)
#define vsnprintf   (int)AsciiVSPrint


#define MAX_STRING_SIZE  0x1000
#define strlen(str)                       (size_t)(AsciiStrnLenS(str,MAX_STRING_SIZE))
#define strcpy(strDest,strSource)         AsciiStrCpyS(strDest,MAX_STRING_SIZE,strSource)
#define strncpy(strDest,strSource,count)  AsciiStrnCpyS(strDest,MAX_STRING_SIZE,strSource,(UINTN)count)
#define strcat(strDest,strSource)         AsciiStrCatS(strDest,MAX_STRING_SIZE,strSource)
#define strncat(strDest,strSource,count)  AsciiStrnCatS(strDest,MAX_STRING_SIZE,strSource,(UINTN)count)
#define strncmp(string1,string2,count)    (int)(AsciiStrnCmp(string1,string2,(UINTN)(count)))
#define strcasecmp(str1,str2)             (int)AsciiStriCmp(str1,str2)
#define strcmp(str1,str2)                 (int)AsciiStrCmp (str1,str2)

void *
malloc (
  size_t  size
  );

void *
realloc (
  void    *ptr,
  size_t  size
  );


void
free (
  void  *ptr
  );

long int labs (long int i);

int abs (int i);

char *strchr(const char *str, int ch);

void* memset (void *dest, char ch, unsigned int count);

#define exit(n)  ASSERT(FALSE);

#endif
