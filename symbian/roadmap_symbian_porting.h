#ifndef _ROADMAP_SYMBIAN_PORTING__H_
#define _ROADMAP_SYMBIAN_PORTING__H_

#ifdef __SYMBIAN32__

#include <stdarg.h>
#include <stddef.h>
int snprintf(char * __restrict str, size_t n, char const * __restrict fmt, ...);
int vsnprintf(char * __restrict str, size_t n, const char * __restrict fmt, va_list ap);

extern int global_FreeMapLock();
extern int global_FreeMapUnlock();

typedef enum 
{
	// Standard edit box
	EEditBoxStandard = 0,
	// Empty forbidden string edit box
	EEditBoxEmptyForbidden,
	// Secured edit box for password
	EEditBoxPassword		
} TEditBoxType;

extern void ShowEditbox( const char* aTitleUtf8, const char* aTextUtf8, int(*aCallback)(int, const char*, void*), void *context, TEditBoxType aBoxType );

#endif

#endif  //   _ROADMAP_SYMBIAN_PORTING__H_
