/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <CryAssert.h>
#include <dirent.h>
#include <vector>
#include <AzCore/std/string/string.h>

/* Memory block identification */
#define _FREE_BLOCK      0
#define _NORMAL_BLOCK    1
#define _CRT_BLOCK       2
#define _IGNORE_BLOCK    3
#define _CLIENT_BLOCK    4
#define _MAX_BLOCKS      5

typedef void* HMODULE;

typedef struct _MEMORYSTATUS
{
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, * LPMEMORYSTATUS;

extern void GlobalMemoryStatus(LPMEMORYSTATUS lpmem);

#if defined(PLATFORM_64BIT)
#   define MEMORY_ALLOCATION_ALIGNMENT 16
#else
#   define MEMORY_ALLOCATION_ALIGNMENT 8
#endif

#define S_OK 0
#define THREAD_PRIORITY_NORMAL              0
#define THREAD_PRIORITY_IDLE                    0
#define THREAD_PRIORITY_LOWEST              0
#define THREAD_PRIORITY_BELOW_NORMAL    0
#define THREAD_PRIORITY_ABOVE_NORMAL    0
#define THREAD_PRIORITY_HIGHEST             0
#define THREAD_PRIORITY_TIME_CRITICAL   0
#define MAX_COMPUTERNAME_LENGTH 15 // required by CryOnline module

#if 0
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
    //CHandle(const PointerType cpHandle) : m_Value(reinterpret_cast<HandleType>(cpHandle)){}
    //CHandle(INVALID_HANDLE_VALUE_ENUM) : m_Value(U){}//to be able to use a common value for all InvalidHandle - types

    operator HandleType(){
        return m_Value;
    }
    bool operator!() const{return m_Value == sciInvalidHandleValue; }
    const CHandle& operator =(const CHandle& crHandle){m_Value = crHandle.m_Value; return *this; }
    const CHandle& operator =(const PointerType cpHandle){m_Value = reinterpret_cast<HandleType>(cpHandle); return *this; }
    const bool operator ==(const CHandle& crHandle)     const{return m_Value == crHandle.m_Value; }
    const bool operator ==(const HandleType cHandle)    const{return m_Value == cHandle; }
    //const bool operator ==(const PointerType cpHandle)const{return m_Value == reinterpret_cast<HandleType>(cpHandle);}
    const bool operator !=(const HandleType cHandle)    const{return m_Value != cHandle; }
    const bool operator !=(const CHandle& crHandle)     const{return m_Value != crHandle.m_Value; }
    //const bool operator !=(const PointerType cpHandle)const{return m_Value != reinterpret_cast<HandleType>(cpHandle);}
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

typedef CHandle<INT_PTR, (INT_PTR) 0> HANDLE;

typedef HANDLE EVENT_HANDLE;
typedef HANDLE THREAD_HANDLE;

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#endif

//#define __TIMESTAMP__ __DATE__" "__TIME__

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

inline void MemoryBarrier() {
    __sync_synchronize();
}
// Memory barrier implementation taken from https://code.google.com/p/gperftools/
//inline void MemoryBarrier() {
//  __asm__ __volatile__("mfence" : : : "memory");
//}

#if !defined(_CPU_SSE)
typedef int64 __m128;
#endif

//////////////////////////////////////////////////////////////////////////
// io.h stuff
#if !defined(ANDROID)
extern int errno;
#endif
typedef unsigned int _fsize_t;

struct _OVERLAPPED;
typedef _OVERLAPPED* LPOVERLAPPED;

typedef void (* LPOVERLAPPED_COMPLETION_ROUTINE)(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, struct _OVERLAPPED* lpOverlapped);

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, * PRECT;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, * PPOINT;

#ifndef _FILETIME_
#define _FILETIME_
typedef struct _FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, * PFILETIME, * LPFILETIME;
#endif

typedef union _ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    };
    unsigned long long QuadPart;
} ULARGE_INTEGER;

typedef ULARGE_INTEGER* PULARGE_INTEGER;

#ifdef __cplusplus
inline LONG CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
#else
static LONG CompareFileTime(const FILETIME* lpFileTime1, const FILETIME* lpFileTime2)
#endif
{
    ULARGE_INTEGER u1, u2;
    memcpy(&u1, lpFileTime1, sizeof u1);
    memcpy(&u2, lpFileTime2, sizeof u2);
    if (u1.QuadPart < u2.QuadPart)
    {
        return -1;
    }
    else
    if (u1.QuadPart > u2.QuadPart)
    {
        return 1;
    }
    return 0;
}

typedef struct _SYSTEMTIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, * PSYSTEMTIME, * LPSYSTEMTIME;

typedef struct _TIME_FIELDS
{
    short Year;
    short Month;
    short Day;
    short Hour;
    short Minute;
    short Second;
    short Milliseconds;
    short Weekday;
} TIME_FIELDS, * PTIME_FIELDS;

#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

inline void ZeroMemory(void* pPtr, int nSize)
{
    memset(pPtr, 0, nSize);
}

inline BOOL InflateRect(RECT* pRect, int dx, int dy)
{
    pRect->left -= dx;
    pRect->right += dx;
    pRect->top -= dy;
    pRect->bottom += dy;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
extern BOOL SystemTimeToFileTime(const SYSTEMTIME* syst, LPFILETIME ft);
//Win32API function declarations actually used
extern bool IsBadReadPtr(void* ptr, unsigned int size);

// Defined in the launcher.
DLL_IMPORT void OutputDebugString(const char*);
DLL_IMPORT void DebugBreak();

/*
//critical section stuff
#define pthread_attr_default NULL

typedef pthread_mutex_t CRITICAL_SECTION;
#ifdef __cplusplus
inline void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
pthread_mutexattr_t pthread_mutexattr_def;
pthread_mutexattr_settype(&pthread_mutexattr_def, PTHREAD_MUTEX_RECURSIVE_NP);
pthread_mutex_init(lpCriticalSection, &pthread_mutexattr_def);
}
inline void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_lock(lpCriticalSection);}
inline void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_unlock(lpCriticalSection);}
inline void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection){}
#else
static void InitializeCriticalSection(CRITICAL_SECTION *lpCriticalSection)
{
pthread_mutexattr_t pthread_mutexattr_def;
pthread_mutexattr_settype(&pthread_mutexattr_def, PTHREAD_MUTEX_RECURSIVE_NP);
pthread_mutex_init(lpCriticalSection, &pthread_mutexattr_def);
}
static void EnterCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_lock(lpCriticalSection);}
static void LeaveCriticalSection(CRITICAL_SECTION *lpCriticalSection){pthread_mutex_unlock(lpCriticalSection);}
static void DeleteCriticalSection(CRITICAL_SECTION *lpCriticalSection){}
#endif
*/

extern bool QueryPerformanceCounter(LARGE_INTEGER* counter);
extern bool QueryPerformanceFrequency(LARGE_INTEGER* frequency);
#ifdef __cplusplus

inline uint32 GetTickCount()
{
    LARGE_INTEGER count, freq;
    QueryPerformanceCounter(&count);
    QueryPerformanceFrequency(&freq);
    return uint32(count.QuadPart * 1000 / freq.QuadPart);
}

#define IGNORE              0       // Ignore signal
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif
//begin--------------------------------findfirst/-next declaration/implementation----------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// function renaming
#define _TRUNCATE (size_t(-1))
#define _chmod chmod
#define _snprintf snprintf
#define stricmp strcasecmp
#define _stricmp strcasecmp
#define strnicmp strncasecmp
#define _strnicmp strncasecmp

#define _strlwr strlwr
#define _strlwr_s(BUF, SIZE) strlwr(BUF)
#define _strups strupr

extern BOOL GetUserName(LPSTR lpBuffer, LPDWORD nSize);

//error code stuff
//not thread specific, just a coarse implementation for the main thread
inline DWORD GetLastError() { return errno; }
inline void SetLastError(DWORD dwErrCode) { errno = dwErrCode; }

//////////////////////////////////////////////////////////////////////////
#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_SHARE_READ                     0x00000001
#define FILE_SHARE_WRITE                    0x00000002
#define OPEN_EXISTING                           3
#define FILE_FLAG_OVERLAPPED            0x40000000
#define INVALID_FILE_SIZE                   ((DWORD)0xFFFFFFFFl)
#define FILE_BEGIN                              0
#define FILE_CURRENT                            1
#define FILE_END                                    2
#define ERROR_NO_SYSTEM_RESOURCES 1450L
#define ERROR_INVALID_USER_BUFFER   1784L
#define ERROR_NOT_ENOUGH_MEMORY   8L
#define ERROR_PATH_NOT_FOUND      3L
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000

//////////////////////////////////////////////////////////////////////////
extern threadID GetCurrentThreadId();

//////////////////////////////////////////////////////////////////////////
extern DWORD Sleep(DWORD dwMilliseconds);

//////////////////////////////////////////////////////////////////////////
extern DWORD SleepEx(DWORD dwMilliseconds, BOOL bAlertable);

extern BOOL GetComputerName(LPSTR lpBuffer, LPDWORD lpnSize); //required for CryOnline
extern DWORD GetCurrentProcessId(void);

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus

//helper function
extern void adaptFilenameToLinux(char* rAdjustedFilename);
extern void replaceDoublePathFilename(char* szFileName);//removes "\.\" to "\" and "/./" to "/"

//////////////////////////////////////////////////////////////////////////
extern void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext);

template <size_t size>
void _makepath_s(char(&path)[size], const char *drive, const char *dir, const char *fname, const char *ext)
{
    _makepath(path, drive, dir, fname, ext);
}

extern void _splitpath(const char* inpath, char* drv, char* dir, char* fname, char* ext);

template <size_t drivesize, size_t dirsize, size_t fnamesize, size_t extsize>
void _splitpath_s(const char *path, char(&drive)[drivesize], char(&dir)[dirsize], char(&fname)[fnamesize], char(&ext)[extsize])
{
    _splitpath(path, drive, dir, fname, ext);
}


//////////////////////////////////////////////////////////////////////////
extern int memicmp(LPCSTR s1, LPCSTR s2, DWORD len);

extern "C" char* strlwr (char* str);
extern "C" char* strupr(char* str);

extern char* _ui64toa(unsigned long long value,    char* str, int radix);
extern long long _atoi64(const char* str);

extern int* _errno();

template <size_t SIZE>
int vsprintf_s(char (&dst)[SIZE], const char* format, va_list argptr)
{
    vsnprintf(dst, SIZE, format, argptr);
    dst[SIZE - 1] = 0;
    return 0;
}

inline int vsprintf_s(char* dst, size_t size, const char* format, va_list argptr)
{
    vsnprintf(dst, size, format, argptr);
    dst[size - 1] = 0;
    return 0;
}

//note that behaviour is different from strncpy.
//this will not pad string with zeroes if length of src is less than 'size'

inline int strncpy_s(char* dst, size_t buf_size, const char* src, size_t size)
{
    size_t to_copy = std::min(size, strlen(src));
    size_t term = std::min(buf_size - 1, to_copy);//length of result
    strncpy(dst, src, term);
    dst[term] = 0;
    return 0;
}

template <size_t SIZE>
inline int strncpy_s(char(&dst)[SIZE], const char* src, size_t size)
{
    size_t to_copy = std::min(size, strlen(src));
    size_t term = std::min(SIZE - 1, to_copy);//length of result
    strncpy(dst, src, term);
    dst[term] = 0;
    return 0;
}

inline int strcat_s(char* dst, size_t size, const char* src)
{
    if (!dst || !src)
    {
        return EINVAL;
    }
    size_t len_dst = strlen(dst);
    size_t len_src = strlen(src);
    if (size == 0 || len_dst + len_src + 1 >= size)
    {
        return ERANGE;
    }
    memcpy(dst + len_dst, src, len_src);
    dst[len_dst + len_src] = 0;
    return 0;
}

template <size_t SIZE>
int strcat_s(char(&dst)[SIZE], const char* src)
{
    return strcat_s(dst, SIZE, src);
}


template <size_t SIZE>
int strcpy_s(char(&dst)[SIZE], const char* src)
{
    strncpy(dst, src, SIZE);
    dst[SIZE - 1] = 0;
    return 0;
}

inline int strcpy_s(char* dst, size_t size, const char* src)
{
    strncpy(dst, src, size);
    dst[size - 1] = 0;
    return 0;
}

inline int sprintf_s(char* buffer, size_t sizeOfBuffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int err = vsnprintf(buffer, sizeOfBuffer, format, args);
    va_end(args);
    return err;
}

template <size_t size>
int sprintf_s(char (&buffer)[size], const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int err = vsnprintf(buffer, size, format, args);
    va_end(args);
    return err;
}

#ifdef _isnan
#undef _isnan
template <typename T>
bool _isnan(T value)
{
    return value != value;
}
#endif


#define vsnprintf_s(BUF, SIZE, COUNT, FORMAT, ARGVLIST) vsnprintf(BUF, SIZE, FORMAT, ARGVLIST)
#define _vsnwprintf_s(BUF, SIZE, COUNT, FORMAT, ARGVLIST) vswprintf(BUF, SIZE, FORMAT, ARGVLIST)
#define fprintf_s fprintf
#define sscanf_s sscanf
#define fread_s(BUF, SIZE, NMEMB, MAX, HANDLE) fread(BUF, SIZE, NMEMB, HANDLE)

//////////////////////////////////////////////////////////////////////////

#ifndef __TRLTOA__
#define __TRLTOA__
extern char* ltoa (long i, char* a, int radix);
#endif
#define itoa ltoa

//////////////////////////////////////////////////////////////////////////
#if 0
inline long int abs(long int x)
{
    return labs(x);
}

inline float abs(float x)
{
    return fabsf(x);
}

inline double abs(double x)
{
    return fabs(x);
}

inline float sqrt(float x)
{
    return sqrtf(x);
}
#else
#include <cmath>
using std::abs;
using std::sqrt;
using std::fabs;
#endif

extern char* _strtime(char* date);
extern char* _strdate(char* date);

#if !defined(_CPU_SSE)
#define _MM_HINT_T0     (1)
#define _MM_HINT_T1     (2)
#define _MM_HINT_T2     (3)
#define _MM_HINT_NTA    (0)
inline void _mm_prefetch(const char*, int) { }
#endif // !_CPU_SSE

#endif //__cplusplus
//////////////////////////////////////////////////////////////////////////
// Byte Swapping functions

inline unsigned short _byteswap_ushort(unsigned short input)
{
    return ((input & 0xff) << 8) | ((input & 0xff00) >> 8);
}

inline LONG _byteswap_ulong(LONG input)
{
    return (input & 0x000000ff) << 24 |
           (input & 0x0000ff00) << 8 |
           (input & 0x00ff0000) >> 8 |
           (input & 0xff000000) >> 24;
}

inline unsigned long long   _byteswap_uint64(unsigned long long input)
{
    return (((input & 0xff00000000000000ull) >> 56) |
            ((input & 0x00ff000000000000ull) >> 40) |
            ((input & 0x0000ff0000000000ull) >> 24) |
            ((input & 0x000000ff00000000ull) >> 8) |
            ((input & 0x00000000ff000000ull) << 8) |
            ((input & 0x0000000000ff0000ull) << 24) |
            ((input & 0x000000000000ff00ull) << 40) |
            ((input & 0x00000000000000ffull) << 56));
}
