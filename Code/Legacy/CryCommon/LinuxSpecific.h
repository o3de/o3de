/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Description : Specific to Linux declarations, inline functions etc.



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

#include <AzCore/base.h>

#define PHYSICS_EXPORTS

#define _PACK __attribute__ ((packed))

#ifndef __cplusplus
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define TCHAR wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif
typedef AZ::u32 COLORREF;
#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((AZ::u16)((BYTE)(g))<<8))|(((AZ::u32)(BYTE)(b))<<16)))

#define GetRValue(rgb)  ((AZ::u8)((AZ::u64)(rgb) & 0xff))
#define GetGValue(rgb)  ((AZ::u8)((AZ::u64)(((AZ::u16)(rgb)) >> 8) & 0xff))
#define GetBValue(rgb)  ((AZ::u8)((AZ::u64)(rgb) & 0xff))

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                \
    ((AZ::u32)(BYTE)(ch0) | ((AZ::u32)(BYTE)(ch1) << 8) | \
     ((AZ::u32)(BYTE)(ch2) << 16) | ((AZ::u32)(BYTE)(ch3) << 24))
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

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
    #define MAX_PATH 256
#endif
#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#define _PTRDIFF_T_DEFINED 1


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

// io.h stuff
typedef unsigned int _fsize_t;

#define _flushall()

struct _OVERLAPPED;


#ifdef __cplusplus

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

// vim:ts=2:sw=2:tw=78
