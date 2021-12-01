/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Specific to Linux declarations, inline functions etc.


#ifndef CRYINCLUDE_CRYCOMMON_LINUXSPECIFIC_H
#define CRYINCLUDE_CRYCOMMON_LINUXSPECIFIC_H
#pragma once


#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <algorithm>
#include <signal.h>
#ifndef __COUNTER__
#define __COUNTER__ __LINE__
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#if defined(APPLE)
#include <dirent.h>
#endif //defined(APPLE)
#if !defined(SKIP_INET_INCLUDES)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // defined(SKIP_INET_INCLUDES)
#include <vector>
#include <string>

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

#ifdef AZ_MONOLITHIC_BUILD
#if !defined(USE_STATIC_NAME_TABLE)
#define USE_STATIC_NAME_TABLE 1
#endif
#endif // AZ_MONOLITHIC_BUILD

#if !defined(_STLP_HASH_MAP) && !defined(USING_STLPORT)
#define _STLP_HASH_MAP 1
#endif

// Enable memory address tracing code.
#if !defined(MM_TRACE_ADDRS) // && !defined(NDEBUG)
#define MM_TRACE_ADDRS 1
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
typedef char TCHAR;

typedef DWORD COLORREF;
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)  (LOBYTE(rgb))
#define GetGValue(rgb)  (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)  (LOBYTE((rgb)>>16))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
     ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#define FILE_ATTRIBUTE_NORMAL               0x00000080

typedef int                         BOOL;
typedef int32_t                 LONG;
typedef uint32_t            ULONG;
typedef int                         HRESULT;

#define BST_UNCHECKED   0x0000

#ifndef MAXUINT
#define MAXUINT ((uint) ~((uint)0))
#endif

#ifndef MAXINT
#define MAXINT ((int)(MAXUINT >> 1))
#endif

#if !defined(__clang__)
typedef int32 __int32;
typedef uint32 __uint32;
typedef int64 __int64;
typedef uint64 __uint64;
#endif

typedef unsigned long int threadID;

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
    #define MAX_PATH 256
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#define _PTRDIFF_T_DEFINED 1

//#define __TIMESTAMP__ __DATE__" "__TIME__

// function renaming
#define _finite __finite
#define _snprintf snprintf
#define _isnan isnan
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp
#define wcsicmp wcscasecmp
#define wcsnicmp wcsncasecmp

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

// stdlib.h stuff
#define _MAX_DRIVE  3   // max. length of drive component
#define _MAX_DIR    256 // max. length of path component
#define _MAX_FNAME  256 // max. length of file name component
#define _MAX_EXT    256 // max. length of extension component

// fcntl.h
#define _O_RDONLY       0x0000  /* open for reading only */
#define _O_WRONLY       0x0001  /* open for writing only */
#define _O_RDWR         0x0002  /* open for reading and writing */
#define _O_APPEND       0x0008  /* writes done at eof */
#define _O_CREAT        0x0100  /* create and open file */
#define _O_TRUNC        0x0200  /* open and truncate */
#define _O_EXCL         0x0400  /* open only if file doesn't already exist */
#define _O_TEXT         0x4000  /* file mode is text (translated) */
#define _O_BINARY       0x8000  /* file mode is binary (untranslated) */
#define _O_RAW  _O_BINARY
#define _O_NOINHERIT    0x0080  /* child process doesn't inherit file */
#define _O_TEMPORARY    0x0040  /* temporary file bit */
#define _O_SHORT_LIVED  0x1000  /* temporary storage file, try not to flush */
#define _O_SEQUENTIAL   0x0020  /* file access is primarily sequential */
#define _O_RANDOM       0x0010  /* file access is primarily random */

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

// io.h stuff
typedef unsigned int _fsize_t;

#define _flushall()

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
    DWORD dwNumberOfBytesTransfered;        //additional member temporary speciying the number of bytes to be read
    /*HANDLE*/ void*  hEvent;
} OVERLAPPED, * LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES
{
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, * PSECURITY_ATTRIBUTES, * LPSECURITY_ATTRIBUTES;

#ifdef __cplusplus
extern bool QueryPerformanceCounter(LARGE_INTEGER*);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);

static pthread_mutex_t mutex_t;
template<typename T>
const volatile T InterlockedIncrement(volatile T* pT)
{
    pthread_mutex_lock(&mutex_t);
    ++(*pT);
    pthread_mutex_unlock(&mutex_t);
    return *pT;
}

template<typename T>
const volatile T InterlockedDecrement(volatile T* pT)
{
    pthread_mutex_lock(&mutex_t);
    --(*pT);
    pthread_mutex_unlock(&mutex_t);
    return *pT;
}

#if 0
template<typename S, typename T>
inline const S& min(const S& rS, const T& rT)
{
    return (rS <= rT) ? rS : rT;
}

template<typename S, typename T>
inline const S& max(const S& rS, const T& rT)
{
    return (rS >= rT) ? rS : rT;
}
#endif

template<typename S, typename T>
inline S __min(const S& rS, const T& rT)
{
    return std::min(rS, rT);
}

template<typename S, typename T>
inline S __max(const S& rS, const T& rT)
{
    return std::max(rS, rT);
}


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
    typedef void* PointerType;      //for compatibility reason to encapsulate a void* as an int

    static const HandleType sciInvalidHandleValue = U;

    CHandle(const CHandle<T, U>& cHandle)
        : m_Value(cHandle.m_Value){}
    CHandle(const HandleType cHandle = U)
        : m_Value(cHandle){}
    CHandle(const PointerType cpHandle)
        : m_Value(reinterpret_cast<HandleType>(cpHandle)){}
    CHandle(INVALID_HANDLE_VALUE_ENUM)
        : m_Value(U){}                                   //to be able to use a common value for all InvalidHandle - types
#if defined(LINUX64) && !defined(__clang__)
    //treat __null tyope also as invalid handle type
    CHandle(__typeof__(__null))
        : m_Value(U){}                            //to be able to use a common value for all InvalidHandle - types
#endif
    operator HandleType(){
        return m_Value;
    }
    bool operator!() const{return m_Value == sciInvalidHandleValue; }
    const CHandle& operator =(const CHandle& crHandle){m_Value = crHandle.m_Value; return *this; }
    const CHandle& operator =(const PointerType cpHandle){m_Value = (HandleType) reinterpret_cast<UINT_PTR>(cpHandle); return *this; }
    const bool operator ==(const CHandle& crHandle)     const{return m_Value == crHandle.m_Value; }
    const bool operator ==(const HandleType cHandle)    const{return m_Value == cHandle; }
    const bool operator ==(const PointerType cpHandle) const{return m_Value == (HandleType) reinterpret_cast<UINT_PTR>(cpHandle); }
    const bool operator !=(const HandleType cHandle)    const{return m_Value != cHandle; }
    const bool operator !=(const CHandle& crHandle)     const{return m_Value != crHandle.m_Value; }
    const bool operator !=(const PointerType cpHandle) const{return m_Value != (HandleType) reinterpret_cast<UINT_PTR>(cpHandle); }
    const bool operator <   (const CHandle& crHandle)       const{return m_Value < crHandle.m_Value; }
    HandleType Handle() const{return m_Value; }

private:
    HandleType m_Value;     //the actual value, remember that file descriptors are ints under linux

    typedef void    ReferenceType;    //for compatibility reason to encapsulate a void* as an int
    //forbid these function which would actually not work on an int
    PointerType operator->();
    PointerType operator->() const;
    ReferenceType operator*();
    ReferenceType operator*() const;
    operator PointerType();
};

typedef CHandle<int, (int) - 1l> HANDLE;

typedef HANDLE EVENT_HANDLE;
typedef pid_t THREAD_HANDLE;

typedef HANDLE HKEY;
// typedef HANDLE HDC;

typedef HANDLE HBITMAP;

typedef HANDLE HMENU;

inline int64 CryGetTicks()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

#endif //__cplusplus

inline int _CrtCheckMemory() { return 1; };

typedef void* HGLRC;
typedef void* HDC;
typedef void* PROC;
typedef void* PIXELFORMATDESCRIPTOR;

template <typename T, size_t N>
char (*RtlpNumberOf( T (&)[N] ))[N];

#define RTL_NUMBER_OF_V2(A) (sizeof(*RtlpNumberOf(A)))

#define ARRAYSIZE(A) RTL_NUMBER_OF_V2(A)

#undef SUCCEEDED
#define SUCCEEDED(x) ((x) >= 0)
#undef FAILED
#define FAILED(x) (!(SUCCEEDED(x)))

#endif // CRYINCLUDE_CRYCOMMON_LINUXSPECIFIC_H

// vim:ts=2:sw=2:tw=78

