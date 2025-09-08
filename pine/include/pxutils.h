#ifndef _PXUTILS_H
#define _PXUTILS_H

#if defined(_WIN32) || defined (_WIN64) || defined(__CYGWIN__)
    #define PX_WINDOWS
    #define PX_WNDCLS_NAME "PineWNDCLS"
    #define PX_WNDCLS_NAME_L L"PineWNDCLS"
#elif defined(__unix__)
    #define PX_UNIX
#else
    #warning "Pine does not support your operating system"
#endif

#define PX_SUCCESS 0

// Errors
#define PX_FAILED_ALLOC 1
#define PX_FAILED_WINDOW_CREATION 2
#define PX_FAILED_HANDLE_FETCH 3
#define PX_FAILED_STRING 4
#define PX_FAILED_OSCALL 5
#define PX_FAILED_VISUAL 6
#define PX_FAILED_FUNCTION_FETCH 7
#define PX_FAILED_OGL_CONTEXT 8

// Warnings
#define PX_INVALID_PARAM 4000
#define PX_UNKNOWN_EVENT 4001

#define PX_TRUE 1
#define PX_FALSE 0

#define PxGetWindowParamI(ctx, win, param) (*(int*)(PxGetWindowParam(ctx, win, param)))

#ifndef PxUint8
#define PxUint8 unsigned char
#endif // PxUint8

#ifndef PxUint16
#define PxUint16 unsigned short
#endif // PxUint16

#ifndef PxUint32
#define PxUint32 unsigned long int
#endif // PxUint32

#ifndef PxUint64
#define PxUint64 unsigned long long int
#endif // PxUint64

#ifndef PxInt8
#define PxInt8 signed char
#endif // PxInt8

#ifndef PxInt16
#define PxInt16 signed short
#endif // PxInt16

#ifndef PxInt32
#define PxInt32 signed long int
#endif // PxInt32

#ifndef PxInt64
#define PxInt64 signed long long int
#endif // PxInt64

typedef PxUint16 PxResult;

#ifndef PxMalloc
#define PX_INCLUDE_STDLIB
#define PxMalloc(...) malloc(__VA_ARGS__)
#endif // PxMalloc

#ifndef PxRealloc
#define PX_INCLUDE_STDLIB
#define PxRealloc(...) realloc(__VA_ARGS__)
#endif // PxRealloc

#ifndef PxFree
#define PX_INCLUDE_STDLIB
#define PxFree(...) free(__VA_ARGS__)
#endif // PxFree

#ifndef PxMemset
#define PX_INCLUDE_STRING
#define PxMemset(...) memset(__VA_ARGS__)
#endif // PxMemset

#ifndef PxMemcpy
#define PX_INCLUDE_STRING
#define PxMemcpy(...) memcpy(__VA_ARGS__)
#endif // PxMemset

#ifndef PxStrlen
#define PX_INCLUDE_STRING
#define PxStrlen(...) strlen(__VA_ARGS__)
#endif // PxMemset

#ifndef PxOffsetof
#define PX_INCLUDE_STDDEF
#define PxOffsetof(...) offsetof(__VA_ARGS__)
#endif // PxOffsetof

#endif //_PXUTILS_H
