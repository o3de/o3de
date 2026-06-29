/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/AzCore_Traits_Platform.h>

// Description : Linux/Mac port support for Win32API calls
#if AZ_TRAIT_LEGACY_CRYCOMMON_USE_WINDOWS_STUBS

#include "platform.h" // Note: This should be first to get consistent debugging definitions

#include <CryCommon/ISystem.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define WINBASE_CPP_SECTION_1 1
#define WINBASE_CPP_SECTION_2 2
#define WINBASE_CPP_SECTION_3 3
#define WINBASE_CPP_SECTION_4 4
#define WINBASE_CPP_SECTION_5 5
#define WINBASE_CPP_SECTION_6 6
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #include <signal.h>
#endif

#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>

#ifdef APPLE
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>                     // for total physical memory on Mac
#include <CoreFoundation/CoreFoundation.h>  // for CryMessageBox
#include <mach/vm_statistics.h>             // host_statistics
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

#include <sys/time.h>

#if defined(LINUX) || defined(APPLE)
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#endif

#if AZ_TRAIT_COMPILER_DEFINE_FS_ERRNO_TYPE
typedef int FS_ERRNO_TYPE;

#include <mutex>

#endif

bool IsBadReadPtr(void* ptr, unsigned int size)
{
    //too complicated to really support it
    return ptr ? false : true;
}

//////////////////////////////////////////////////////////////////////////
char* _strtime(char* date)
{
    azstrcpy(date, AZ_ARRAY_SIZE(date), "0:0:0");
    return date;
}

//////////////////////////////////////////////////////////////////////////
char* _strdate(char* date)
{
    azstrcpy(date, AZ_ARRAY_SIZE(date), "0");
    return date;
}

//////////////////////////////////////////////////////////////////////////
char* strlwr (char* str)
{
    char* cp;             /* traverses string for C locale conversion */

    for (cp = str; *cp; ++cp)
    {
        if ('A' <= *cp && *cp <= 'Z')
        {
            *cp += 'a' - 'A';
        }
    }
    return str;
}

char* strupr (char* str)
{
    char* cp;             /* traverses string for C locale conversion */

    for (cp = str; *cp; ++cp)
    {
        if ('a' <= *cp && *cp <= 'z')
        {
            *cp += 'A' - 'a';
        }
    }
    return str;
}

char* ltoa (long i, char* a, int radix)
{
    if (a == nullptr)
    {
        return nullptr;
    }
    strcpy (a, "0");
    if (i && radix > 1 && radix < 37)
    {
        char buf[35];
        unsigned long u = i, p = 34;
        buf[p] = 0;
        if (i < 0 && radix == 10)
        {
            u = -i;
        }
        while (u)
        {
            unsigned int d = u % radix;
            buf[--p] = d < 10 ? '0' + d : 'a' + d - 10;
            u /= radix;
        }
        if (i < 0 && radix == 10)
        {
            buf[--p] = '-';
        }
        strcpy (a, buf + p);
    }
    return a;
}


#if AZ_TRAIT_COMPILER_DEFINE_WCSICMP
// For Linux it's redefined to wcscasecmp and wcsncasecmp'
int wcsicmp (const wchar_t* s1, const wchar_t* s2)
{
    wint_t c1, c2;

    if (s1 == s2)
    {
        return 0;
    }

    do
    {
        c1 = towlower(*s1++);
        c2 = towlower(*s2++);
    }
    while (c1 && c1 == c2);

    return (int) (c1 - c2);
}

int wcsnicmp (const wchar_t* s1, const wchar_t* s2, size_t count)
{
    wint_t c1, c2;
    if (s1 == s2 || count == 0)
    {
        return 0;
    }

    do
    {
        c1 = towlower(*s1++);
        c2 = towlower(*s2++);
    }
    while ((--count) && c1 && (c1 == c2));
    return (int) (c1 - c2);
}
#endif

#if defined(ANDROID)
// not defined in android-19 or prior
size_t wcsnlen(const wchar_t* str, size_t maxLen)
{
    size_t length;
    for (length = 0; length < maxLen; ++length, ++str)
    {
        if (!*str)
        {
            break;
        }
    }
    return length;
}

#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
bool QueryPerformanceCounter(LARGE_INTEGER* counter)
{
#if defined(LINUX)
    // replaced gettimeofday
    // http://fixunix.com/kernel/378888-gettimeofday-resolution-linux.html
    timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    counter->QuadPart = (uint64)tv.tv_sec * 1000000 + tv.tv_nsec / 1000;
    return true;
#elif defined(APPLE)
    counter->QuadPart = mach_absolute_time();
    return true;
#else
    return false;
#endif
}

bool QueryPerformanceFrequency(LARGE_INTEGER* frequency)
{
#if defined(LINUX)
    // On Linux we'll use gettimeofday().  The API resolution is microseconds,
    // so we'll report that to the caller.
    frequency->u.LowPart  = 1000000;
    frequency->u.HighPart = 0;
    return true;
#elif defined(APPLE)
    static mach_timebase_info_data_t s_kTimeBaseInfoData;
    if (s_kTimeBaseInfoData.denom == 0)
    {
        mach_timebase_info(&s_kTimeBaseInfoData);
    }
    // mach_timebase_info_data_t expresses the tick period in nanoseconds
    frequency->QuadPart = 1e+9 * (uint64_t)s_kTimeBaseInfoData.denom / (uint64_t)s_kTimeBaseInfoData.numer;
    return true;
#else
    return false;
#endif
}
#endif

//////////////////////////////////////////////////////////////////////////
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

#endif

#include <chrono>
#include <thread>

//////////////////////////////////////////////////////////////////////////
AZ::u32 Sleep(AZ::u32 dwMilliseconds)
{
#if defined(LINUX) || defined(APPLE)
    std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
    return 0;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_5
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    timeval tv, start, now;
    uint64 tStart;

    memset(&tv, 0, sizeof tv);
    memset(&start, 0, sizeof start);
    memset(&now, 0, sizeof now);
    gettimeofday(&now, NULL);
    start = now;
    tStart = (uint64)start.tv_sec * 1000000 + start.tv_usec;
    while (true)
    {
        uint64 tNow, timePassed, timeRemaining;
        tNow = (uint64)now.tv_sec * 1000000 + now.tv_usec;
        timePassed = tNow - tStart;
        if (timePassed >= dwMilliseconds)
        {
            break;
        }
        timeRemaining = dwMilliseconds * 1000 - timePassed;
        tv.tv_sec = timeRemaining / 1000000;
        tv.tv_usec = timeRemaining % 1000000;
        select(1, NULL, NULL, NULL, &tv);
        gettimeofday(&now, NULL);
    }
    return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CrySleep(unsigned int dwMilliseconds)
{
    Sleep(dwMilliseconds);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryMessageBox(const char* lpText, const char* lpCaption, [[maybe_unused]] unsigned int uType)
{
#ifdef WIN32
#   error WIN32 is defined in WinBase.cpp (it is a non-Windows file)
#elif defined(MAC)
    CFStringRef strText = CFStringCreateWithCString(NULL, lpText, kCFStringEncodingMacRoman);
    CFStringRef strCaption = CFStringCreateWithCString(NULL, lpCaption, kCFStringEncodingMacRoman);

    CFStringRef strOk = CFSTR("OK");
    CFStringRef strCancel = CFSTR("Cancel");
    CFStringRef strRetry = CFSTR("Retry");
    CFStringRef strYes = CFSTR("Yes");
    CFStringRef strNo = CFSTR("No");
    CFStringRef strAbort = CFSTR("Abort");
    CFStringRef strIgnore = CFSTR("Ignore");
    CFStringRef strTryAgain = CFSTR("Try Again");
    CFStringRef strContinue = CFSTR("Continue");

    CFStringRef defaultButton = nullptr;
    CFStringRef alternativeButton = nullptr;
    CFStringRef otherButton = nullptr;

    switch (uType & 0xf)
    {
        case MB_OKCANCEL:
            defaultButton = strOk;
            alternativeButton = strCancel;
            break;
        case MB_ABORTRETRYIGNORE:
            defaultButton = strAbort;
            alternativeButton = strRetry;
            otherButton = strIgnore;
            break;
        case MB_YESNOCANCEL:
            defaultButton = strYes;
            alternativeButton = strNo;
            otherButton = strCancel;
            break;
        case MB_YESNO:
            defaultButton = strYes;
            alternativeButton = strNo;
            break;
        case MB_RETRYCANCEL:
            defaultButton = strRetry;
            alternativeButton = strCancel;
            break;
        case MB_CANCELTRYCONTINUE:
            defaultButton = strCancel;
            alternativeButton = strTryAgain;
            otherButton = strContinue;
            break;
        case MB_OK:
        default:
            defaultButton = strOk;
            break;
    }

    CFOptionFlags kResult;
    CFUserNotificationDisplayAlert(
        0,                                 // no timeout
        kCFUserNotificationNoteAlertLevel, //change it depending message_type flags ( MB_ICONASTERISK.... etc.)
        NULL,                              //icon url, use default, you can change it depending message_type flags
        NULL,                              //not used
        NULL,                              //localization of strings
        strText,                           //header text
        strCaption,                        //message text
        defaultButton,                     //default "ok" text in button
        alternativeButton,                 //alternate button title
        otherButton,                       //other button title, null--> no other button
        &kResult                           //response flags
    );

    if (strCaption)
    {
        CFRelease(strCaption);
    }
    if (strText)
    {
        CFRelease(strText);
    }

#else
    printf("Messagebox: cap: %s  text:%s\n", lpCaption ? lpCaption : " ", lpText ? lpText : " ");
#endif
}

#if defined(LINUX) || defined(APPLE)

#endif//LINUX APPLE

#if defined(APPLE) || defined(LINUX)
// WinAPI debug functions.
AZ_DLL_EXPORT void OutputDebugString(const char* outputString)
{
#if !defined(_RELEASE)
    // Emulates dev tools output in Xcode and cmd line launch with idevicedebug.
    fprintf(stdout, "%s", outputString);
#endif
}

#endif

#endif // AZ_TRAIT_LEGACY_CRYCOMMON_USE_WINDOWS_STUBS
