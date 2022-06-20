/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Apple specific declarations common amongst its products

#ifndef CRYINCLUDE_CRYCOMMON_APPLESPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_APPLESPECIFIC_H
#pragma once

//////////////////////////////////////////////////////////////////////////
// Standard includes.
//////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <malloc/malloc.h>
#include <Availability.h>
// Atomic operations , guaranteed to work across all apple platforms
#include <libkern/OSAtomic.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
//////////////////////////////////////////////////////////////////////////

#define FP16_MESH
#define BOOST_DISABLE_WIN32

#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif

typedef void*                               LPVOID;
#define VOID                    void
#define PVOID                               void*

typedef unsigned int UINT;
typedef char CHAR;
typedef float FLOAT;

#define PHYSICS_EXPORTS

// MSVC compiler-specific keywords
#define __forceinline inline
#define _inline inline
#define __cdecl
#define _cdecl
#define __stdcall
#define _stdcall
#define __fastcall
#define _fastcall
#define IN
#define OUT

#define MAP_ANONYMOUS MAP_ANON

//////////////////////////////////////////////////////////////////////////
// Define platform independent types.
//////////////////////////////////////////////////////////////////////////
#include "BaseTypes.h"

typedef double real;

typedef uint32          DWORD;
typedef DWORD*          LPDWORD;
typedef uint64                          DWORD_PTR;
typedef intptr_t INT_PTR, * PINT_PTR;
typedef uintptr_t UINT_PTR, * PUINT_PTR;
typedef char* LPSTR, * PSTR;
typedef char TCHAR;
typedef uint64          __uint64;
#if !defined(__clang__)
typedef int64               __int64;
#endif
typedef int64               INT64;
typedef uint64          UINT64;

typedef long LONG_PTR, * PLONG_PTR, * PLONG;
typedef unsigned long ULONG_PTR, * PULONG_PTR;

typedef uint8                               BYTE;
typedef uint16                          WORD;
typedef void*                               HWND;
typedef UINT_PTR                        WPARAM;
typedef LONG_PTR                        LPARAM;
typedef LONG_PTR                        LRESULT;
#define PLARGE_INTEGER LARGE_INTEGER *
typedef const char* LPCSTR, * PCSTR;
typedef long long                       LONGLONG;
typedef ULONG_PTR                       SIZE_T;
typedef uint8                               byte;
#define ILINE __forceinline

#ifndef MAXUINT
#define MAXUINT ((uint) ~((uint)0))
#endif

#ifndef MAXINT
#define MAXINT ((int)(MAXUINT >> 1))
#endif

#ifndef _CVTBUFSIZE
#define _CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop */
#endif

#ifndef STDMETHODCALLTYPE_DEFINED
#define STDMETHODCALLTYPE_DEFINED
#define STDMETHODCALLTYPE
#endif

#define _PACK __attribute__ ((packed))

// Safe memory freeing
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)          { if (p) { delete (p);       (p) = NULL; } \
}
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)    { if (p) { delete[] (p);     (p) = NULL; } \
}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)         { if (p) { (p)->Release();   (p) = NULL; } \
}
#endif


#define MAKEWORD(a, b)      ((WORD)(((BYTE)((DWORD_PTR)(a) & 0xff)) | ((WORD)((BYTE)((DWORD_PTR)(b) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)((DWORD_PTR)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD_PTR)(b) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD_PTR)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD_PTR)(w) >> 8))

#define CALLBACK
#define WINAPI

#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define TCHAR wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character
typedef WCHAR* PWCHAR;
typedef WCHAR* LPWCH, * PWCH;
typedef const WCHAR* LPCWCH, * PCWCH;
typedef WCHAR* NWPSTR;
typedef WCHAR* LPWSTR, * PWSTR;
typedef WCHAR* LPUWSTR, * PUWSTR;

typedef const WCHAR* LPCWSTR, * PCWSTR;
typedef const WCHAR* LPCUWSTR, * PCUWSTR;

typedef LPCWSTR LPCTSTR;
typedef LPWSTR LPTSTR;

typedef DWORD COLORREF;
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)  (LOBYTE(rgb))
#define GetGValue(rgb)  (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)  (LOBYTE((rgb)>>16))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#define FILE_ATTRIBUTE_NORMAL               0x00000080

// Conflit with OBJC defined bool type.

#if defined(IOS)
typedef bool BOOL;
#else
typedef signed char BOOL;
#endif

typedef int32_t LONG;
typedef unsigned int ULONG;
typedef int HRESULT;

//typedef int32 __int32;
typedef uint32 __uint32;
typedef int64 __int64;
typedef uint64 __uint64;

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#define _PTRDIFF_T_DEFINED 1

typedef union _LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    };
    struct
    {
        DWORD LowPart;
        LONG HighPart;
    } u;
    long long QuadPart;
} LARGE_INTEGER;

#define _A_RDONLY       (0x01)    /* Read only file */
#define _A_HIDDEN (0x02)    /* Hidden file */
#define _A_SUBDIR       (0x10)    /* Subdirectory */

//////////////////////////////////////////////////////////////////////////
// Win32 FileAttributes.
//////////////////////////////////////////////////////////////////////////
#define FILE_ATTRIBUTE_READONLY             0x00000001
#define FILE_ATTRIBUTE_HIDDEN               0x00000002
#define FILE_ATTRIBUTE_SYSTEM               0x00000004
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020
#define FILE_ATTRIBUTE_DEVICE               0x00000040
#define FILE_ATTRIBUTE_NORMAL               0x00000080
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800
#define FILE_ATTRIBUTE_OFFLINE              0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000

#define INVALID_FILE_ATTRIBUTES (-1)

#define BST_UNCHECKED   0x0000

#ifndef HRESULT_VALUES_DEFINED
#define HRESULT_VALUES_DEFINED
enum
{
    E_OUTOFMEMORY  = 0x8007000E,
    E_FAIL         = 0x80004005,
    E_ABORT        = 0x80004004,
    E_INVALIDARG   = 0x80070057,
    E_NOINTERFACE  = 0x80004002,
    E_NOTIMPL      = 0x80004001,
    E_UNEXPECTED   = 0x8000FFFF
};
#endif

#define ERROR_SUCCESS   0L

enum
{
    IDOK        = 1,
    IDCANCEL    = 2,
    IDABORT     = 3,
    IDRETRY     = 4,
    IDIGNORE    = 5,
    IDYES       = 6,
    IDNO        = 7,
    IDTRYAGAIN  = 10,
    IDCONTINUE  = 11
};

#define MB_OK                0x00000000L
#define MB_OKCANCEL          0x00000001L
#define MB_ABORTRETRYIGNORE  0x00000002L
#define MB_YESNOCANCEL       0x00000003L
#define MB_YESNO             0x00000004L
#define MB_RETRYCANCEL       0x00000005L
#define MB_CANCELTRYCONTINUE 0x00000006L

#define MB_ICONQUESTION     0x00000020L
#define MB_ICONEXCLAMATION  0x00000030L
    
#define MB_ICONERROR        0x00000010L
#define MB_ICONWARNING      0x00000030L
#define MB_ICONINFORMATION  0x00000040L

#define MB_SETFOREGROUND    0x00010000L

#define MB_APPLMODAL    0x00000000L

#define MK_LBUTTON  0x0001
#define MK_RBUTTON  0x0002
#define MK_SHIFT    0x0004
#define MK_CONTROL  0x0008
#define MK_MBUTTON  0x0010

#define SM_MOUSEPRESENT 0x00000000L

#define SM_CMOUSEBUTTONS    43

#define VK_TAB      0x09
#define VK_SHIFT    0x10
#define VK_MENU     0x12
#define VK_ESCAPE   0x1B
#define VK_SPACE    0x20
#define VK_DELETE   0x2E

#define VK_OEM_COMMA    0xBC   // ',' any country
#define VK_OEM_PERIOD   0xBE   // '.' any country
#define VK_OEM_3        0xC0   // '`~' for US
#define VK_OEM_4        0xDB  //  '[{' for US
#define VK_OEM_6        0xDD  //  ']}' for US

#define WAIT_TIMEOUT 258L    // dderror

#define WM_MOVE 0x0003
#define WM_USER 0x0400

#define WHEEL_DELTA 120

#define WS_CHILD    0x40000000L
#define WS_VISIBLE  0x10000000L

#define CB_ERR  (-1)

// function renaming
#define _finite std::isfinite
#define _snprintf snprintf
//#define _isnan isnan
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define wcsicmp wcscasecmp
#define wcsnicmp wcsncasecmp
//#define memcpy_s(dest,bytes,src,n) memcpy(dest,src,n)
#define _isnan ISNAN

#define TARGET_DEFAULT_ALIGN (0x8U)

#define _msize malloc_size


struct _OVERLAPPED;

typedef void (* LPOVERLAPPED_COMPLETION_ROUTINE)(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, struct _OVERLAPPED* lpOverlapped);

typedef struct _OVERLAPPED
{
    void* pCaller;//this is orginally reserved for internal purpose, we store the Caller pointer here
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine; ////this is orginally ULONG_PTR InternalHigh and reserved for internal purpose
    union
    {
        struct
        {
            DWORD Offset;
            DWORD OffsetHigh;
        };
        PVOID Pointer;
    };
    DWORD dwNumberOfBytesTransfered;    //additional member temporary speciying the number of bytes to be read
    /*HANDLE*/ void*  hEvent;
} OVERLAPPED, * LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES
{
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, * LPSECURITY_ATTRIBUTES;

#ifdef __cplusplus

#define __min(_S, _T) min(_S, _T)
#define __max(_S, _T) max(_S, _T)

typedef enum
{
    INVALID_HANDLE_VALUE = -1l
}INVALID_HANDLE_VALUE_ENUM;
//for compatibility reason we got to create a class which actually contains an int rather than a void* and make sure it does not get mistreated
template <class T, T U>
//U is default type for invalid handle value, T the encapsulated handle type to be used instead of void* (as under windows and never linux)
class CHandle
{
public:
    typedef T           HandleType;
    typedef void* PointerType;  //for compatibility reason to encapsulate a void* as an int

    static const HandleType sciInvalidHandleValue = U;

    CHandle(const CHandle<T, U>& cHandle)
        : m_Value(cHandle.m_Value){}
    CHandle(const HandleType cHandle = U)
        : m_Value(cHandle){}
    CHandle(const PointerType cpHandle)
        : m_Value(reinterpret_cast<HandleType>(cpHandle)){}
    CHandle(INVALID_HANDLE_VALUE_ENUM)
        : m_Value(U){}                               //to be able to use a common value for all InvalidHandle - types
#if defined(PLATFORM_64BIT)
    //treat __null tyope also as invalid handle type
    CHandle(long)
        : m_Value(U){}          //to be able to use a common value for all InvalidHandle - types
#endif
    operator HandleType(){
        return m_Value;
    }
    bool operator!() const{return m_Value == sciInvalidHandleValue; }
    const CHandle& operator =(const CHandle& crHandle){m_Value = crHandle.m_Value; return *this; }
    const CHandle& operator =(const PointerType cpHandle){m_Value = (HandleType) reinterpret_cast<UINT_PTR>(cpHandle); return *this; }
    const bool operator ==(const CHandle& crHandle)     const{return m_Value == crHandle.m_Value; }
    const bool operator ==(const HandleType cHandle)    const{return m_Value == cHandle; }
    const bool operator ==(const PointerType cpHandle) const{return m_Value == reinterpret_cast<HandleType>(cpHandle); }
    const bool operator !=(const HandleType cHandle)    const{return m_Value != cHandle; }
    const bool operator !=(const CHandle& crHandle)     const{return m_Value != crHandle.m_Value; }
    const bool operator !=(const PointerType cpHandle) const{return m_Value != reinterpret_cast<HandleType>(cpHandle); }
    const bool operator <   (const CHandle& crHandle)       const{return m_Value < crHandle.m_Value; }
    HandleType Handle() const{return m_Value; }

private:
    HandleType m_Value; //the actual value, remember that file descriptors are ints under linux

    typedef void    ReferenceType;//for compatibility reason to encapsulate a void* as an int
    //forbid these function which would actually not work on an int
    PointerType operator->();
    PointerType operator->() const;
    ReferenceType operator*();
    ReferenceType operator*() const;
    operator PointerType();
};

typedef CHandle<int, (int) - 1l> HANDLE;

typedef HANDLE EVENT_HANDLE;
typedef HANDLE THREAD_HANDLE;

typedef HANDLE HKEY;
typedef HANDLE HDC;

typedef HANDLE HBITMAP;

typedef HANDLE HMENU;

#endif //__cplusplus

extern bool QueryPerformanceCounter(LARGE_INTEGER*);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);

inline int64 CryGetTicks()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

#ifdef _RELEASE
#define __debugbreak()
#else
#define __debugbreak() ::raise(SIGTRAP)
#endif

#define __assume(x)
#define _flushall sync

inline int closesocket(int s)
{
    return ::close(s);
}

template <typename T, size_t N>
char (*RtlpNumberOf( T (&)[N] ))[N];

#define RTL_NUMBER_OF_V2(A) (sizeof(*RtlpNumberOf(A)))

#define ARRAYSIZE(A) RTL_NUMBER_OF_V2(A)

#undef SUCCEEDED
#define SUCCEEDED(x) ((x) >= 0)
#undef FAILED
#define FAILED(x) (!(SUCCEEDED(x)))

#endif // CRYINCLUDE_CRYCOMMON_APPLESPECIFIC_H
