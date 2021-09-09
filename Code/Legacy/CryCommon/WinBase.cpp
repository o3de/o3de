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
#include <CryAssert.h>

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

#if defined(ANDROID)
#define FIX_FILENAME_CASE 0 // everything is lower case on android
#elif defined(LINUX) || defined(APPLE)
#define FIX_FILENAME_CASE 1
#endif

#include <sys/time.h>



#if !defined(_RELEASE) || defined(_DEBUG)
#include <set>
unsigned int g_EnableMultipleAssert = 0;//set to something else than 0 if to enable already reported asserts
#endif

#if defined(LINUX) || defined(APPLE)
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>
#include "CryLibrary.h"
#endif

#if defined(APPLE)
    #include <AzFramework/Utils/SystemUtilsApple.h>
#endif

#if AZ_TRAIT_COMPILER_DEFINE_FS_ERRNO_TYPE
typedef int FS_ERRNO_TYPE;
#if AZ_TRAIT_COMPILER_DEFINE_FS_STAT_TYPE
typedef struct stat FS_STAT_TYPE;
#else
typedef struct stat64 FS_STAT_TYPE;
#endif

#include <mutex>

#elif AZ_TRAIT_COMPILER_DEFINE_FS_STAT_TYPE
#error cannot request AZ_TRAIT_COMPILER_DEFINE_FS_STAT_TYPE if AZ_TRAIT_COMPILER_DEFINE_FS_ERRNO_TYPE is zero
#endif

#if AZ_TRAIT_COMPILER_DEFINE_SASSERTDATA_TYPE && (!defined(_RELEASE) || defined(_DEBUG))
struct SAssertData
{
    int line;
    char fileName[256 - sizeof(int)];
    const bool operator==(const SAssertData& crArg) const
    {
        return crArg.line == line && (strcmp(fileName, crArg.fileName) == 0);
    }

    const bool operator<(const SAssertData& crArg) const
    {
        if (line == crArg.line)
        {
            return strcmp(fileName, crArg.fileName) < 0;
        }
        else
        {
            return line < crArg.line;
        }
    }

    SAssertData()
        : line(-1){}
    SAssertData(const int cLine, const char* cpFile)
        : line(cLine)
    {
        azstrcpy(fileName, AZ_ARRAY_SIZE(fileName), cpFile);
    }

    SAssertData(const SAssertData& crAssertData)
    {
        memcpy((void*)this, &crAssertData, sizeof(SAssertData));
    }

    void operator=(const SAssertData& crAssertData)
    {
        memcpy((void*)this, &crAssertData, sizeof(SAssertData));
    }
};


//#define OUTPUT_ASSERT_TO_FILE

void HandleAssert(const char* cpMessage, const char* cpFunc, const char* cpFile, const int cLine)
{
#if defined(OUTPUT_ASSERT_TO_FILE)
    static FILE* pAssertLogFile = nullptr;
    if (!pAssertLogFile)
    {
        azfopen(&pAssertLogFile, "Assert.log", "w+");
    }
#endif
    bool report = true;
    static std::set<SAssertData> assertSet;
    SAssertData assertData(cLine, cpFile);
    if (!g_EnableMultipleAssert)
    {
        std::set<SAssertData>::const_iterator it = assertSet.find(assertData);
        if (it != assertSet.end())
        {
            report = false;
        }
        else
        {
            assertSet.insert(assertData);
        }
    }
    else
    {
        assertSet.insert(assertData);
    }
    if (report)
    {
        //added function to be able to place a breakpoint here or to print out to other consoles
        printf("ASSERT: %s in %s (%s : %d)\n", cpMessage, cpFunc, cpFile, cLine);
#if defined(OUTPUT_ASSERT_TO_FILE)
        if (pAssertLogFile)
        {
            fprintf(pAssertLogFile, "ASSERT: %s in %s (%s : %d)\n", cpMessage, cpFunc, cpFile, cLine);
            fflush(pAssertLogFile);
        }
#endif
    }
}
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
    if (a == NULL)
    {
        return NULL;
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

char* stpcpy(char* dest, const char* str)
{
    while (*str != '\0')
    {
        *dest++ = *str++;
    }
    *dest = '\0';

    return dest;
}
#endif

void _makepath(char* path, const char* drive, const char* dir, const char* filename, const char* ext)
{
    char ch;
    char tmp[MAX_PATH];
    if (!path)
    {
        return;
    }
    tmp[0] = '\0';
    if (drive && drive[0])
    {
        tmp[0] = drive[0];
        tmp[1] = ':';
        tmp[2] = 0;
    }
    if (dir && dir[0])
    {
        azstrcat(tmp, MAX_PATH, dir);
        ch = tmp[strlen(tmp) - 1];
        if (ch != '/' && ch != '\\')
        {
            azstrcat(tmp, MAX_PATH, "\\");
        }
    }
    if (filename && filename[0])
    {
        azstrcat(tmp, MAX_PATH, filename);
        if (ext && ext[0])
        {
            if (ext[0] != '.')
            {
                azstrcat(tmp, MAX_PATH, ".");
            }
            azstrcat(tmp, MAX_PATH, ext);
        }
    }
    azstrcpy(path, strlen(tmp) + 1, tmp);
}

char* _ui64toa(unsigned long long value,   char* str, int radix)
{
    if (str == 0)
    {
        return 0;
    }

    char buffer[65];
    char* pos;
    int digit;

    pos = &buffer[64];
    *pos = '\0';

    do
    {
        digit = value % radix;
        value = value / radix;
        if (digit < 10)
        {
            *--pos = '0' + digit;
        }
        else
        {
            *--pos = 'a' + digit - 10;
        } /* if */
    } while (value != 0L);

    memcpy(str, pos, &buffer[64] - pos + 1);
    return str;
}

long long _atoi64(const char* str)
{
    if (str == 0)
    {
        return -1;
    }
    unsigned long long RunningTotal = 0;
    char bMinus = 0;
    while (*str == ' ' || (*str >= '\011' && *str <= '\015'))
    {
        str++;
    } /* while */
    if (*str == '+')
    {
        str++;
    }
    else if (*str == '-')
    {
        bMinus = 1;
        str++;
    } /* if */
    while (*str >= '0' && *str <= '9')
    {
        RunningTotal = RunningTotal * 10 + *str - '0';
        str++;
    } /* while */
    return bMinus ? ((long long)-RunningTotal) : (long long)RunningTotal;
}

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

void _splitpath(const char* inpath, char* drv, char* dir, char* fname, char* ext)
{
    if (drv)
    {
        drv[0] = 0;
    }

    typedef AZStd::fixed_string<AZ_MAX_PATH_LEN> path_stack_string;

    const path_stack_string inPath(inpath);
    AZStd::string::size_type s = inPath.rfind('/', inPath.size());//position of last /
    path_stack_string fName;
    if (s == AZStd::string::npos)
    {
        if (dir)
        {
            dir[0] = 0;
        }
        fName = inpath; //assign complete string as rest
    }
    else
    {
        if (dir)
        {
            azstrcpy(dir, AZ_MAX_PATH_LEN, (inPath.substr((AZStd::string::size_type)0, (AZStd::string::size_type)(s + 1))).c_str());    //assign directory
        }
        fName = inPath.substr((AZStd::string::size_type)(s + 1));                    //assign remaining string as rest
    }
    if (fName.size() == 0)
    {
        if (ext)
        {
            ext[0] = 0;
        }
        if (fname)
        {
            fname[0] = 0;
        }
    }
    else
    {
        //dir and drive are now set
        s = fName.find(".", (AZStd::string::size_type)0);//position of first .
        if (s == AZStd::string::npos)
        {
            if (ext)
            {
                ext[0] = 0;
            }
            if (fname)
            {
                azstrcpy(fname, fName.size() + 1, fName.c_str());   //assign filename
            }
        }
        else
        {
            if (ext)
            {
                azstrcpy(ext, AZ_MAX_PATH_LEN, (fName.substr(s)).c_str());     //assign extension including .
            }
            if (fname)
            {
                if (s == 0)
                {
                    fname[0] = 0;
                }
                else
                {
                    azstrcpy(fname, AZ_MAX_PATH_LEN, (fName.substr((AZStd::string::size_type)0, s)).c_str());  //assign filename
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int memicmp(LPCSTR s1, LPCSTR s2, DWORD len)
{
    int ret = 0;
    while (len--)
    {
        if ((ret = tolower(*s1) - tolower(*s2)))
        {
            break;
        }
        s1++;
        s2++;
    }
    return ret;
}

//-----------------------------------------other stuff-------------------------------------------------------------------

void GlobalMemoryStatus(LPMEMORYSTATUS lpmem)
{
    //not complete implementation
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_3
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE)

    // Retrieve dwTotalPhys
    int kMIB[] = {CTL_HW, HW_MEMSIZE};
    uint64_t dwTotalPhys;
    size_t ulength = sizeof(dwTotalPhys);
    if (sysctl(kMIB, 2, &dwTotalPhys, &ulength, NULL, 0) != 0)
    {
        gEnv->pLog->LogError("sysctl failed\n");
    }
    else
    {
        lpmem->dwTotalPhys = static_cast<SIZE_T>(dwTotalPhys);
    }

    // Get the page size
    mach_port_t kHost(mach_host_self());
    vm_size_t uPageSize;
    if (host_page_size(kHost, &uPageSize) != 0)
    {
        gEnv->pLog->LogError("host_page_size failed\n");
    }
    else
    {
        // Get memory statistics
        vm_statistics_data_t kVMStats;
        mach_msg_type_number_t uCount(sizeof(kVMStats) / sizeof(natural_t));
        if (host_statistics(kHost, HOST_VM_INFO, (host_info_t)&kVMStats, &uCount) != 0)
        {
            gEnv->pLog->LogError("host_statistics failed\n");
        }
        else
        {
            // Calculate dwAvailPhys
            lpmem->dwAvailPhys = uPageSize * kVMStats.free_count;
        }
    }
#else
    FILE* f;
    lpmem->dwMemoryLoad    = 0;
    lpmem->dwTotalPhys     = 16 * 1024 * 1024;
    lpmem->dwAvailPhys     = 16 * 1024 * 1024;
    lpmem->dwTotalPageFile = 16 * 1024 * 1024;
    lpmem->dwAvailPageFile = 16 * 1024 * 1024;
    azfopen(&f, "/proc/meminfo", "r");
    if (f)
    {
        char buffer[256];
        memset(buffer, '0', 256);
        int total, used, free, shared, buffers, cached;

        lpmem->dwLength = sizeof(MEMORYSTATUS);
        lpmem->dwTotalPhys = lpmem->dwAvailPhys = 0;
        lpmem->dwTotalPageFile = lpmem->dwAvailPageFile = 0;
        while (fgets(buffer, sizeof(buffer), f))
        {
            if (azsscanf(buffer, "Mem: %d %d %d %d %d %d", &total, &used, &free, &shared, &buffers, &cached))
            {
                lpmem->dwTotalPhys += total;
                lpmem->dwAvailPhys += free + buffers + cached;
            }
            if (azsscanf(buffer, "Swap: %d %d %d", &total, &used, &free))
            {
                lpmem->dwTotalPageFile += total;
                lpmem->dwAvailPageFile += free;
            }
            if (azsscanf(buffer, "MemTotal: %d", &total))
            {
                lpmem->dwTotalPhys = total * 1024;
            }
            if (azsscanf(buffer, "MemFree: %d", &free))
            {
                lpmem->dwAvailPhys = free * 1024;
            }
            if (azsscanf(buffer, "SwapTotal: %d", &total))
            {
                lpmem->dwTotalPageFile = total * 1024;
            }
            if (azsscanf(buffer, "SwapFree: %d", &free))
            {
                lpmem->dwAvailPageFile = free * 1024;
            }
            if (azsscanf(buffer, "Buffers: %d", &buffers))
            {
                lpmem->dwAvailPhys += buffers * 1024;
            }
            if (azsscanf(buffer, "Cached: %d", &cached))
            {
                lpmem->dwAvailPhys += cached * 1024;
            }
        }
        fclose(f);
        if (lpmem->dwTotalPhys)
        {
            DWORD TotalPhysical = lpmem->dwTotalPhys + lpmem->dwTotalPageFile;
            DWORD AvailPhysical = lpmem->dwAvailPhys + lpmem->dwAvailPageFile;
            lpmem->dwMemoryLoad = (TotalPhysical - AvailPhysical)  / (TotalPhysical / 100);
        }
    }
#endif
}

static const int YearLengths[2] = {DAYSPERNORMALYEAR, DAYSPERLEAPYEAR};
static const int MonthLengths[2][MONSPERYEAR] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

static void NormalizeTimeFields(short* FieldToNormalize, short* CarryField, int Modulus)
{
    *FieldToNormalize = (short) (*FieldToNormalize - Modulus);
    *CarryField = (short) (*CarryField + 1);
}

bool TimeFieldsToTime(PTIME_FIELDS tfTimeFields, PLARGE_INTEGER Time)
{
    #define SECSPERMIN         60
    #define MINSPERHOUR        60
    #define HOURSPERDAY        24
    #define MONSPERYEAR        12

    #define EPOCHYEAR          1601

    #define SECSPERDAY         86400
    #define TICKSPERMSEC       10000
    #define TICKSPERSEC        10000000
    #define SECSPERHOUR        3600

    int CurYear, CurMonth;
    LONGLONG rcTime;
    TIME_FIELDS TimeFields = *tfTimeFields;

    rcTime = 0;
    while (TimeFields.Second >= SECSPERMIN)
    {
        NormalizeTimeFields(&TimeFields.Second, &TimeFields.Minute, SECSPERMIN);
    }
    while (TimeFields.Minute >= MINSPERHOUR)
    {
        NormalizeTimeFields(&TimeFields.Minute, &TimeFields.Hour, MINSPERHOUR);
    }
    while (TimeFields.Hour >= HOURSPERDAY)
    {
        NormalizeTimeFields(&TimeFields.Hour, &TimeFields.Day, HOURSPERDAY);
    }
    while (TimeFields.Day > MonthLengths[IsLeapYear(TimeFields.Year)][TimeFields.Month - 1])
    {
        NormalizeTimeFields(&TimeFields.Day, &TimeFields.Month, SECSPERMIN);
    }
    while (TimeFields.Month > MONSPERYEAR)
    {
        NormalizeTimeFields(&TimeFields.Month, &TimeFields.Year, MONSPERYEAR);
    }
    for (CurYear = EPOCHYEAR; CurYear < TimeFields.Year; CurYear++)
    {
        rcTime += YearLengths[IsLeapYear(CurYear)];
    }
    for (CurMonth = 1; CurMonth < TimeFields.Month; CurMonth++)
    {
        rcTime += MonthLengths[IsLeapYear(CurYear)][CurMonth - 1];
    }
    rcTime += TimeFields.Day - 1;
    rcTime *= SECSPERDAY;
    rcTime += TimeFields.Hour * SECSPERHOUR + TimeFields.Minute * SECSPERMIN + TimeFields.Second;

    rcTime *= TICKSPERSEC;
    rcTime += TimeFields.Milliseconds * TICKSPERMSEC;

    Time->QuadPart = rcTime;

    return true;
}

BOOL SystemTimeToFileTime(const SYSTEMTIME* syst, LPFILETIME ft)
{
    TIME_FIELDS tf;
    LARGE_INTEGER t;

    tf.Year = syst->wYear;
    tf.Month = syst->wMonth;
    tf.Day = syst->wDay;
    tf.Hour = syst->wHour;
    tf.Minute = syst->wMinute;
    tf.Second = syst->wSecond;
    tf.Milliseconds = syst->wMilliseconds;

    TimeFieldsToTime(&tf, &t);
    ft->dwLowDateTime = t.u.LowPart;
    ft->dwHighDateTime = t.u.HighPart;
    return TRUE;
}

void adaptFilenameToLinux(AZStd::string& rAdjustedFilename)
{
    //first replace all \\ by /
    AZStd::string::size_type loc = 0;
    while ((loc = rAdjustedFilename.find("\\", loc)) != AZStd::string::npos)
    {
        rAdjustedFilename.replace(loc, 1, "/");
    }
    loc = 0;
    //remove /./
    while ((loc = rAdjustedFilename.find("/./", loc)) != AZStd::string::npos)
    {
        rAdjustedFilename.replace(loc, 3, "/");
    }
}

void replaceDoublePathFilename(char* szFileName)
{
    //replace "\.\" by "\"
    AZStd::string s(szFileName);
    AZStd::string::size_type loc = 0;
    //remove /./
    while ((loc = s.find("/./", loc)) != AZStd::string::npos)
    {
        s.replace(loc, 3, "/");
    }
    loc = 0;
    //remove "\.\"
    while ((loc = s.find("\\.\\", loc)) != AZStd::string::npos)
    {
        s.replace(loc, 3, "\\");
    }
    azstrcpy((char*)szFileName, AZ_MAX_PATH_LEN, s.c_str());
}

#if FIX_FILENAME_CASE
static bool FixOnePathElement(char* path)
{
    if (*path == '\0')
    {
        return true;
    }

    if ((path[0] == '/') && (path[1] == '\0'))
    {
        return true;  // root dir always exists.
    }
    if (strchr(path, '*') || strchr(path, '?'))
    {
        return true; // wildcard...stop correcting path.
    }
    struct stat statbuf;
    if (stat(path, &statbuf) != -1)      // current case exists.
    {
        return true;
    }

    char* name = path;
    char* ptr = strrchr(path, '/');
    if (ptr)
    {
        name = ptr + 1;
        *ptr = '\0';
    }

    if (*name == '\0')      // trailing '/' ?
    {
        *ptr = '/';
        return true;
    }

    const char* parent;
    if (ptr == path)
    {
        parent = "/";
    }
    else if (ptr == NULL)
    {
        parent = ".";
    }
    else
    {
        parent = path;
    }

    DIR* dirp = opendir(parent);
    if (ptr)
    {
        *ptr = '/';
    }

    if (dirp == NULL)
    {
        return false;
    }

    struct dirent* dent;
    bool found = false;
    while ((dent = readdir(dirp)) != NULL)
    {
        if (strcasecmp(dent->d_name, name) == 0)
        {
            azstrcpy(name, AZ_MAX_PATH_LEN, dent->d_name);
            found = true;
            break;
        }
    }

    closedir(dirp);
    return found;
}
#endif

#define Int32x32To64(a, b) ((uint64)((uint64)(a)) * (uint64)((uint64)(b)))

//////////////////////////////////////////////////////////////////////////
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION WINBASE_CPP_SECTION_4
    #include AZ_RESTRICTED_FILE(WinBase_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
threadID GetCurrentThreadId()
{
    return threadID(pthread_self());
}
#endif

//////////////////////////////////////////////////////////////////////////
DWORD Sleep(DWORD dwMilliseconds)
{
#if defined(LINUX) || defined(APPLE)
    timespec req;
    timespec rem;

    memset(&req, 0, sizeof(req));
    memset(&rem, 0, sizeof(rem));

    time_t sec = (int)(dwMilliseconds / 1000);
    req.tv_sec = sec;
    req.tv_nsec = (dwMilliseconds - (sec * 1000)) * 1000000L;
    if (nanosleep(&req, &rem) == -1)
    {
        nanosleep(&rem, 0);
    }

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
DWORD SleepEx(DWORD dwMilliseconds, BOOL bAlertable)
{
    //TODO: implement
    //  CRY_ASSERT_MESSAGE(0, "SleepEx not implemented yet");
    printf("SleepEx not properly implemented yet\n");
    Sleep(dwMilliseconds);
    return 0;
}

#if defined(LINUX) || defined(APPLE)
BOOL GetComputerName(LPSTR lpBuffer, LPDWORD lpnSize)
{
    if (!lpBuffer || !lpnSize)
    {
        return FALSE;
    }

    int err = gethostname(lpBuffer, *lpnSize);

    if (-1 == err)
    {
        CryLog("GetComputerName falied [%d]\n", errno);
        return FALSE;
    }
    return TRUE;
}
#endif

#if AZ_TRAIT_COMPILER_DEFINE_GETCURRENTPROCESSID
DWORD GetCurrentProcessId(void)
{
    return (DWORD)getpid();
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CrySleep(unsigned int dwMilliseconds)
{
    Sleep(dwMilliseconds);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
int CryMessageBox(const char* lpText, const char* lpCaption, unsigned int uType)
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

    if (kResult == kCFUserNotificationDefaultResponse)
    {
        switch (uType & 0xf)
        {
            case MB_OK:
            case MB_OKCANCEL:
            default:
                return IDOK;
            case MB_ABORTRETRYIGNORE:
                return IDABORT;
            case MB_YESNOCANCEL:
            case MB_YESNO:
                return IDYES;
            case MB_RETRYCANCEL:
                return IDRETRY;
            case MB_CANCELTRYCONTINUE:
                return IDCANCEL;
        }
    }
    else if (kResult == kCFUserNotificationAlternateResponse)
    {
        switch (uType & 0xf)
        {
            case MB_OKCANCEL:
            case MB_RETRYCANCEL:
                return IDCANCEL;
            case MB_ABORTRETRYIGNORE:
                return IDRETRY;
            case MB_YESNOCANCEL:
            case MB_YESNO:
                return IDNO;
            case MB_CANCELTRYCONTINUE:
                return IDTRYAGAIN;
            default:
                assert(false);
                return IDCANCEL;
        }
    }
    else if (kResult == kCFUserNotificationOtherResponse)
    {
        switch (uType & 0xf)
        {
            case MB_ABORTRETRYIGNORE:
                return IDIGNORE;
            case MB_YESNOCANCEL:
                return IDCANCEL;
            case MB_CANCELTRYCONTINUE:
                return IDCONTINUE;
            default:
                assert(false);
                return IDCANCEL;
        }
    }
    return 0;
#else
    printf("Messagebox: cap: %s  text:%s\n", lpCaption ? lpCaption : " ", lpText ? lpText : " ");
    return 0;
#endif
}

#if defined(LINUX) || defined(APPLE)

threadID CryGetCurrentThreadId()
{
    return GetCurrentThreadId();
}

#endif//LINUX APPLE

#if defined(APPLE) || defined(LINUX)
// WinAPI debug functions.
DLL_EXPORT void OutputDebugString(const char* outputString)
{
#if !defined(_RELEASE)
    // Emulates dev tools output in Xcode and cmd line launch with idevicedebug.
    fprintf(stdout, "%s", outputString);
#endif
}

#endif

// This code does not have a long life span and will be replaced soon
#if defined(APPLE) || defined(LINUX) || defined(DEFINE_LEGACY_CRY_FILE_OPERATIONS)

typedef DIR* FS_DIR_TYPE;
typedef dirent FS_DIRENT_TYPE;
static const FS_ERRNO_TYPE FS_ENOENT = ENOENT;
static const FS_DIR_TYPE FS_DIR_NULL = NULL;

typedef int FS_ERRNO_TYPE;

#if defined(APPLE)
typedef struct stat FS_STAT_TYPE;
#else
typedef struct stat64 FS_STAT_TYPE;
#endif

#include <mutex>

bool CrySetFileAttributes(const char* lpFileName, uint32 dwFileAttributes)
{
    //TODO: implement
    printf("CrySetFileAttributes not properly implemented yet\n");
    return false;
}


ILINE void FS_OPEN(const char* szFileName, int iFlags, int& iFileDesc, mode_t uMode, FS_ERRNO_TYPE& rErr)
{
    rErr = ((iFileDesc = open(szFileName, iFlags, uMode)) != -1) ? 0 : errno;
}

ILINE void FS_CLOSE(int iFileDesc, FS_ERRNO_TYPE& rErr)
{
    rErr = close(iFileDesc) != -1 ? 0 : errno;
}

ILINE void FS_CLOSE_NOERR(int iFileDesc)
{
    close(iFileDesc);
}

ILINE void FS_OPENDIR(const char* szDirName, FS_DIR_TYPE& pDir, FS_ERRNO_TYPE& rErr)
{
    rErr = (pDir = opendir(szDirName)) != NULL ? 0 : errno;
}

ILINE void FS_READDIR(FS_DIR_TYPE pDir, FS_DIRENT_TYPE& kEnt, uint64_t& uEntSize, FS_ERRNO_TYPE& rErr)
{
    errno = 0;  // errno is used to determine if readdir succeeds after
    FS_DIRENT_TYPE* pDirent(readdir(pDir));
    if (pDirent == NULL)
    {
        uEntSize = 0;
        rErr = (errno == FS_ENOENT) ? 0 : errno;
    }
    else
    {
        kEnt = *pDirent;
        uEntSize = static_cast<uint64_t>(sizeof(FS_DIRENT_TYPE));
        rErr = 0;
    }
}

ILINE void FS_STAT(const char* szFileName, FS_STAT_TYPE& kStat, FS_ERRNO_TYPE& rErr)
{
#if defined(APPLE)
    rErr = stat(szFileName, &kStat) != -1 ? 0 : errno;
#else
    rErr = stat64(szFileName, &kStat) != -1 ? 0 : errno;
#endif
}

ILINE void FS_FSTAT(int iFileDesc, FS_STAT_TYPE& kStat, FS_ERRNO_TYPE& rErr)
{
#if defined(APPLE)
    rErr = fstat(iFileDesc, &kStat) != -1 ? 0 : errno;
#else
    rErr = fstat64(iFileDesc, &kStat) != -1 ? 0 : errno;
#endif
}

ILINE void FS_CLOSEDIR(FS_DIR_TYPE pDir, FS_ERRNO_TYPE& rErr)
{
    errno = 0;
    rErr = closedir(pDir) == 0 ? 0 : errno;
}

ILINE void FS_CLOSEDIR_NOERR(FS_DIR_TYPE pDir)
{
    closedir(pDir);
}

const bool GetFilenameNoCase
(
    const char* file,
    char* pAdjustedFilename,
    const bool cCreateNew
)
{
    assert(file);
    assert(pAdjustedFilename);
    azstrcpy(pAdjustedFilename, AZ_MAX_PATH_LEN, file);

    // Fix the dirname case.
    const int cLen = strlen(file);
    for (int i = 0; i < cLen; ++i)
    {
        if (pAdjustedFilename[i] == '\\')
        {
            pAdjustedFilename[i] = '/';
        }
    }

    char* slash;
    const char* dirname;
    char* name;

    if ((pAdjustedFilename) == (char*)-1)
    {
        return false;
    }

    slash = strrchr(pAdjustedFilename, '/');
    if (slash)
    {
        dirname = pAdjustedFilename;
        name = slash + 1;
        *slash = 0;
    }
    else
    {
        dirname = ".";
        name = pAdjustedFilename;
    }

#if !defined(LINUX) && !defined(APPLE) && !defined(DEFINE_SKIP_WILDCARD_CHECK)      // fix the parent path anyhow.
    // Check for wildcards. We'll always return true if the specified filename is
    // a wildcard pattern.
    if (strchr(name, '*') || strchr(name, '?'))
    {
        if (slash)
        {
            *slash = '/';
        }
        return true;
    }
#endif

    // Scan for the file.
    if (slash)
    {
        *slash = '/';
    }

#if FIX_FILENAME_CASE
    char* path = pAdjustedFilename;
    char* sep;
    while ((sep = strchr(path, '/')) != NULL)
    {
        *sep = '\0';
        const bool exists = FixOnePathElement(pAdjustedFilename);
        *sep = '/';
        if (!exists)
        {
            return false;
        }

        path = sep + 1;
    }
    if (!FixOnePathElement(pAdjustedFilename)) // catch last filename.
    {
        return false;
    }

#else
    for (char* c = pAdjustedFilename; *c; ++c)
    {
        *c = tolower(*c);
    }
#endif

    return true;
}

DWORD GetFileAttributes(LPCWSTR lpFileNameW)
{
    AZStd::string lpFileName;
    AZStd::to_string(lpFileName, lpFileNameW);
    struct stat fileStats;
    const int success = stat(lpFileName.c_str(), &fileStats);
    if (success == -1)
    {
        char adjustedFilename[MAX_PATH];
        GetFilenameNoCase(lpFileName.c_str(), adjustedFilename);
        if (stat(adjustedFilename, &fileStats) == -1)
        {
            return (DWORD)INVALID_FILE_ATTRIBUTES;
        }
    }
    DWORD ret = 0;

    const int acc = (fileStats.st_mode & S_IWRITE);

    if (acc != 0)
    {
        if (S_ISDIR(fileStats.st_mode) != 0)
        {
            ret |= FILE_ATTRIBUTE_DIRECTORY;
        }
    }
    return (ret == 0) ? FILE_ATTRIBUTE_NORMAL : ret;//return file attribute normal as the default value, must only be set if no other attributes have been found
}

__finddata64_t::~__finddata64_t()
{
    if (m_Dir != FS_DIR_NULL)
    {
        FS_CLOSEDIR_NOERR(m_Dir);
        m_Dir = FS_DIR_NULL;
    }
}
#endif //defined(APPLE) || defined(LINUX)

#endif // AZ_TRAIT_LEGACY_CRYCOMMON_USE_WINDOWS_STUBS
