/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySystem_precompiled.h"
#include "System.h"
#include "AutoDetectSpec.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CPUDETECT_CPP_SECTION_1 1
#define CPUDETECT_CPP_SECTION_2 2
#endif

#if defined(WIN32)
#include <intrin.h>
#elif defined(LINUX) || defined(APPLE)
#include <sys/resource.h>   // setrlimit, getrlimit
#endif

#if defined(APPLE)
#include <mach/mach.h>
#include <mach/mach_init.h>         // mach_thread_self
#include <mach/thread_policy.h>     // Mac OS Thread affinity API
#include <sys/sysctl.h>
#endif

#if defined(LINUX)
#ifndef __GNU_SOURCE
#define __GNU_SOURCE
#endif
#include <pthread.h> //already includes sched.h
#endif

/* features */
#define FPU_FLAG  0x0001
#define SERIAL_FLAG 0x40000
#define MMX_FLAG  0x800000
#define ISSE_FLAG 0x2000000

#ifdef __GNUC__
# define cpuid(op, eax, ebx, ecx, edx) __asm__("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (op) : "cc");
#endif

int g_CpuFlags;

struct SAutoMaxPriority
{
    SAutoMaxPriority()
    {
        /* get a copy of the current thread and process priorities */
#if defined(WIN32)
        priority_class = GetPriorityClass(GetCurrentProcess());
        thread_priority = GetThreadPriority(GetCurrentThread());
#elif defined(LINUX) || defined(APPLE)
        nice_priority = getpriority(PRIO_PROCESS, 0);
        success = nice_priority >= 0 &&
            pthread_getschedparam(pthread_self(), &thread_policy, &thread_sched_param) == 0;
#endif

        /* make this thread the highest possible priority */
#if defined(WIN32)
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(LINUX) || defined(APPLE)
        if (success)
        {
            setpriority(PRIO_PROCESS, 0, MAX_NICE_PRIORITY);

            sched_param new_sched_param = thread_sched_param;
            new_sched_param.sched_priority = sched_get_priority_max(thread_policy);
            pthread_setschedparam(pthread_self(), thread_policy, &new_sched_param);
        }
#endif
    }

    ~SAutoMaxPriority()
    {
        /* restore the thread priority */
#if defined(WIN32)
        SetPriorityClass(GetCurrentProcess(), priority_class);
        SetThreadPriority(GetCurrentThread(), thread_priority);
#elif defined(LINUX) || defined(APPLE)
        if (success)
        {
            pthread_setschedparam(pthread_self(), thread_policy, &thread_sched_param);
            setpriority(PRIO_PROCESS, 0, nice_priority);
        }
#endif
    }

#if defined(WIN32)
    uint32  priority_class;
    int   thread_priority;
#elif defined(LINUX) || defined(APPLE)
    rlimit nice_limit;
    int nice_priority;
    int thread_policy;
    sched_param thread_sched_param;
    bool success;
    enum
    {
        MAX_NICE_PRIORITY = 40
    };
#endif
};

#if AZ_LEGACY_CRYSYSTEM_TRAIT_ASM_VOLATILE_CPUID
static inline void __cpuid(int CPUInfo[4], int InfoType)
{
    asm volatile("cpuid" : "=a" (*CPUInfo), "=b" (*(CPUInfo + 1)), "=c" (*(CPUInfo + 2)), "=d" (*(CPUInfo + 3)) : "a" (InfoType));
}
#endif

bool IsAMD()
{
// Broken out for validation support.
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
    #define AZ_SUPPORTS_AMD
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CPUDETECT_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(CPUDetect_cpp)
#endif

#if defined(AZ_SUPPORTS_AMD)
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000000);

    char szCPU[13];
    memset(szCPU, 0, sizeof(szCPU));
    *(int*)&szCPU[0] = CPUInfo[1];
    *(int*)&szCPU[4] = CPUInfo[3];
    *(int*)&szCPU[8] = CPUInfo[2];

    return (strcmp(szCPU, "AuthenticAMD") == 0);
#else
    return false;
#endif
}

bool IsIntel()
{
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000000);

    char szCPU[13];
    memset(szCPU, 0, sizeof(szCPU));
    *(int*)&szCPU[0] = CPUInfo[1];
    *(int*)&szCPU[4] = CPUInfo[3];
    *(int*)&szCPU[8] = CPUInfo[2];

    return (strcmp(szCPU, "GenuineIntel") == 0);
#else
    return false;
#endif
}

bool Has64bitExtension()
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HAS64BITEXT
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x80000001);   // Argument "Processor Signature and AMD Features"
    if (CPUInfo[3] & 0x20000000)        // Bit 29 in edx is set if 64-bit address extension is supported
    {
        return true;
    }
    else
    {
        return false;
    }
#elif defined(WIN64) || defined(LINUX64) || defined(MAC)
    return true;
#else
    return false;
#endif
}

bool HTSupported()
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HTSUPPORTED
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    if (CPUInfo[3] & 0x10000000)        // Bit 28 in edx is set if HT is supported
    {
        return true;
    }
    else
    {
        return false;
    }
#else
    return false;
#endif
}

uint8 LogicalProcPerPhysicalProc()
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    // Bits 16-23 in ebx contain the number of logical processors per physical processor when execute cpuid with eax set to 1
    return (uint8) ((CPUInfo[1] & 0x00FF0000) >> 16);
#else
    return 1;
#endif
}

uint8 GetAPIC_ID()
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    // Bits 24-31 in ebx contain the unique initial APIC ID for the processor this code is running on. Default value = 0xff if HT is not supported.
    return (uint8) ((CPUInfo[1] & 0xFF000000) >> 24);
#else
    return 0;
#endif
}

void GetCPUName(char* pName)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
    if (pName)
    {
        int CPUInfo[4];
        __cpuid(CPUInfo, 0x80000000);
        if (CPUInfo[0] >= 0x80000004)
        {
            __cpuid(CPUInfo, 0x80000002);
            ((int*)pName)[0] = CPUInfo[0];
            ((int*)pName)[1] = CPUInfo[1];
            ((int*)pName)[2] = CPUInfo[2];
            ((int*)pName)[3] = CPUInfo[3];

            __cpuid(CPUInfo, 0x80000003);
            ((int*)pName)[4] = CPUInfo[0];
            ((int*)pName)[5] = CPUInfo[1];
            ((int*)pName)[6] = CPUInfo[2];
            ((int*)pName)[7] = CPUInfo[3];

            __cpuid(CPUInfo, 0x80000004);
            ((int*)pName)[8] = CPUInfo[0];
            ((int*)pName)[9] = CPUInfo[1];
            ((int*)pName)[10] = CPUInfo[2];
            ((int*)pName)[11] = CPUInfo[3];
        }
        else
        {
            pName[0] = '\0';
        }
    }
#else
    if (pName)
    {
        pName[0] = '\0';
    }
#endif
}

bool HasFPUOnChip()
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    // Bit 0 in edx indicates presents of on chip FPU
    return (CPUInfo[3] & 0x00000001) != 0;
#else
    return false;
#endif
}

void GetCPUSteppingModelFamily(int& stepping, int& model, int& family)
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    stepping = CPUInfo[0] & 0xF; // Bit 0-3 in eax specifies stepping
    model = (CPUInfo[0] >> 4) & 0xF; // Bit 4-7 in eax specifies model
    family = (CPUInfo[0] >> 8) & 0xF; // Bit 8-11 in eax specifies family
#else
    stepping = 0;
    model = 0;
    family = 0;
#endif
}

#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID
unsigned long GetCPUFeatureSet()
{
    unsigned long features = 0;

    int CPUInfo[4];

    __cpuid(CPUInfo, 0);
    const int nIds = CPUInfo[0];

    __cpuid(CPUInfo, 0x80000000);
    const unsigned int nExIds = CPUInfo[0];

    if (nIds > 0)
    {
        __cpuid(CPUInfo, 0x00000001);

        if (CPUInfo[3] & (1 << 26))
        {
            features |= CFI_SSE2;
        }
        if (CPUInfo[3] & (1 << 25))
        {
            features |= CFI_SSE;
        }
        if (CPUInfo[2] & (1 << 0))
        {
            features |= CFI_SSE3;
        }
        if (CPUInfo[2] & (1 << 29))
        {
            features |= CFI_F16C;
        }
        if (CPUInfo[2] & (1 << 19))
        {
            features |= CFI_SSE41;
        }
    }

    if (nExIds > 0x80000000)
    {
        __cpuid(CPUInfo, 0x80000001);
        if (CPUInfo[3] & (1 << 31))
        {
            features |= CFI_3DNOW;
        }
    }

    return features;
}
#endif

#if AZ_LEGACY_CRYSYSTEM_TRAIT_DEFINE_DETECT_PROCESSOR
static unsigned long __stdcall DetectProcessor(void* arg)
{
    const char hex_chars[16] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    unsigned long signature = 0;
    unsigned long cache_temp;
    unsigned long cache_eax = 0;
    unsigned long cache_ebx = 0;
    unsigned long cache_ecx = 0;
    unsigned long cache_edx = 0;
    unsigned long features_edx = 0;
    unsigned long serial_number[3];
    unsigned char cpu_type;
    unsigned char fpu_type;
    unsigned char CPUID_flag = 0;
    unsigned char celeron_flag = 0;
    unsigned char pentiumxeon_flag = 0;
    unsigned char amd3d_flag = 0;
    unsigned char name_flag = 0;
    char  vendor[13];
    vendor[0] = '\0';
    char  name[49];
    name[0] = '\0';
    char* serial;
    const char* cpu_string;
    const char* cpu_extra_string;
    const char* fpu_string;
    const char* vendor_string;
    SCpu* p = (SCpu*) arg;

    memset(p, 0, sizeof(*p));

    if (IsAMD() && Has64bitExtension())
    {
        p->meVendor = eCVendor_AMD;
        p->mFeatures |= GetCPUFeatureSet();
        p->mbSerialPresent = false;
        azstrcpy(p->mSerialNumber, AZ_ARRAY_SIZE(p->mSerialNumber), "");
        GetCPUSteppingModelFamily(p->mStepping, p->mModel, p->mFamily);
        azstrcpy(p->mVendor, AZ_ARRAY_SIZE(p->mVendor), "AMD");
        GetCPUName(p->mCpuType);
        azstrcpy(p->mFpuType, AZ_ARRAY_SIZE(p->mFpuType), HasFPUOnChip() ? "On-Chip" : "Unknown");
        p->mbPhysical = true;

        return 1;
    }
    else if (IsIntel() && Has64bitExtension())
    {
        p->meVendor = eCVendor_Intel;
        p->mFeatures |= GetCPUFeatureSet();
        p->mbSerialPresent = false;
        azstrcpy(p->mSerialNumber, AZ_ARRAY_SIZE(p->mSerialNumber), "");
        GetCPUSteppingModelFamily(p->mStepping, p->mModel, p->mFamily);
        azstrcpy(p->mVendor, AZ_ARRAY_SIZE(p->mVendor), "Intel");
        GetCPUName(p->mCpuType);
        azstrcpy(p->mFpuType, AZ_ARRAY_SIZE(p->mFpuType), HasFPUOnChip() ? "On-Chip" : "Unknown");

        p->mbPhysical = true;

        return 1;
    }

    cpu_type = 0xF;
    fpu_type = 3;
    signature = 0;

    p->mFamily = cpu_type;
    p->mModel = (signature >> 4) & 0xf;
    p->mStepping = signature & 0xf;

    p->mFeatures = 0;

    p->mFeatures |= amd3d_flag ? CFI_3DNOW : 0;
    p->mFeatures |= (features_edx & MMX_FLAG)  ? CFI_MMX : 0;
    p->mFeatures |= (features_edx & ISSE_FLAG) ? CFI_SSE : 0;
    p->mbSerialPresent = ((features_edx & SERIAL_FLAG) != 0);

    if (features_edx & SERIAL_FLAG)
    {
        serial_number[0] = serial_number[1] = serial_number[2] = 0;

        /* format number */
        serial = p->mSerialNumber;

        serial[0] = hex_chars[(serial_number[2] >> 28) & 0x0f];
        serial[1] = hex_chars[(serial_number[2] >> 24) & 0x0f];
        serial[2] = hex_chars[(serial_number[2] >> 20) & 0x0f];
        serial[3] = hex_chars[(serial_number[2] >> 16) & 0x0f];

        serial[4] = '-';

        serial[5] = hex_chars[(serial_number[2] >> 12) & 0x0f];
        serial[6] = hex_chars[(serial_number[2] >>  8) & 0x0f];
        serial[7] = hex_chars[(serial_number[2] >>  4) & 0x0f];
        serial[8] = hex_chars[(serial_number[2] >>  0) & 0x0f];

        serial[9] = '-';

        serial[10] = hex_chars[(serial_number[1] >> 28) & 0x0f];
        serial[11] = hex_chars[(serial_number[1] >> 24) & 0x0f];
        serial[12] = hex_chars[(serial_number[1] >> 20) & 0x0f];
        serial[13] = hex_chars[(serial_number[1] >> 16) & 0x0f];

        serial[14] = '-';

        serial[15] = hex_chars[(serial_number[1] >> 12) & 0x0f];
        serial[16] = hex_chars[(serial_number[1] >>  8) & 0x0f];
        serial[17] = hex_chars[(serial_number[1] >>  4) & 0x0f];
        serial[18] = hex_chars[(serial_number[1] >>  0) & 0x0f];

        serial[19] = '-';

        serial[20] = hex_chars[(serial_number[0] >> 28) & 0x0f];
        serial[21] = hex_chars[(serial_number[0] >> 24) & 0x0f];
        serial[22] = hex_chars[(serial_number[0] >> 20) & 0x0f];
        serial[23] = hex_chars[(serial_number[0] >> 16) & 0x0f];

        serial[24] = '-';

        serial[25] = hex_chars[(serial_number[0] >> 12) & 0x0f];
        serial[26] = hex_chars[(serial_number[0] >>  8) & 0x0f];
        serial[27] = hex_chars[(serial_number[0] >>  4) & 0x0f];
        serial[28] = hex_chars[(serial_number[0] >>  0) & 0x0f];

        serial[29] = 0;
    }

    vendor_string = "Unknown";
    cpu_string = "Unknown";
    cpu_extra_string = "";
    fpu_string = "Unknown";

    if (!CPUID_flag)
    {
        switch (cpu_type)
        {
        case 0:
            cpu_string = "8086";
            break;

        case 2:
            cpu_string = "80286";
            break;

        case 3:
            cpu_string = "80386";
            switch (fpu_type)
            {
            case 2:
                fpu_string = "80287";
                break;

            case 1:
                fpu_string = "80387";
                break;

            default:
                fpu_string = "None";
                break;
            }
            break;

        case 4:
            if (fpu_type)
            {
                cpu_string = "80486DX, 80486DX2 or 80487SX";
                fpu_string = "on-chip";
            }
            else
            {
                cpu_string = "80486SX";
            }
            break;
        }
    }
    else
    {  /* using CPUID instruction */
        if (!name_flag)
        {
            if (!strcmp(vendor, "GenuineIntel"))
            {
                vendor_string = "Intel";
                switch (cpu_type)
                {
                case 4:
                    switch (p->mModel)
                    {
                    case 0:
                    case 1:
                        cpu_string = "80486DX";
                        break;

                    case 2:
                        cpu_string = "80486SX";
                        break;

                    case 3:
                        cpu_string = "80486DX2";
                        break;

                    case 4:
                        cpu_string = "80486SL";
                        break;

                    case 5:
                        cpu_string = "80486SX2";
                        break;

                    case 7:
                        cpu_string = "Write-Back Enhanced 80486DX2";
                        break;

                    case 8:
                        cpu_string = "80486DX4";
                        break;

                    default:
                        cpu_string = "80486";
                    }
                    break;

                case 5:
                    switch (p->mModel)
                    {
                    default:
                    case 1:
                    case 2:
                    case 3:
                        cpu_string = "Pentium";
                        break;

                    case 4:
                        cpu_string = "Pentium MMX";
                        break;
                    }
                    break;

                case 6:
                    switch (p->mModel)
                    {
                    case 1:
                        cpu_string = "Pentium Pro";
                        break;

                    case 3:
                        cpu_string = "Pentium II";
                        break;

                    case 5:
                    case 7:
                    {
                        cache_temp = cache_eax & 0xFF000000;
                        if (cache_temp == 0x40000000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_eax & 0xFF0000;
                        if (cache_temp == 0x400000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_eax & 0xFF00;
                        if (cache_temp == 0x4000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ebx & 0xFF000000;
                        if (cache_temp == 0x40000000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ebx & 0xFF0000;
                        if (cache_temp == 0x400000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ebx & 0xFF00;
                        if (cache_temp == 0x4000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ebx & 0xFF;
                        if (cache_temp == 0x40)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44) && (cache_temp <= 0x45))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ecx & 0xFF000000;
                        if (cache_temp == 0x40000000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ecx & 0xFF0000;
                        if (cache_temp == 0x400000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ecx & 0xFF00;
                        if (cache_temp == 0x4000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_ecx & 0xFF;
                        if (cache_temp == 0x40)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44) && (cache_temp <= 0x45))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_edx & 0xFF000000;
                        if (cache_temp == 0x40000000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44000000) && (cache_temp <= 0x45000000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_edx & 0xFF0000;
                        if (cache_temp == 0x400000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x440000) && (cache_temp <= 0x450000))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_edx & 0xFF00;
                        if (cache_temp == 0x4000)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x4400) && (cache_temp <= 0x4500))
                        {
                            pentiumxeon_flag = 1;
                        }
                        cache_temp = cache_edx & 0xFF;
                        if (cache_temp == 0x40)
                        {
                            celeron_flag = 1;
                        }
                        if ((cache_temp >= 0x44) && (cache_temp <= 0x45))
                        {
                            pentiumxeon_flag = 1;
                        }

                        if (celeron_flag)
                        {
                            cpu_string = "Celeron";
                        }
                        else
                        {
                            if (pentiumxeon_flag)
                            {
                                if (p->mModel == 5)
                                {
                                    cpu_string = "Pentium II Xeon";
                                }
                                else
                                {
                                    cpu_string = "Pentium III Xeon";
                                }
                            }
                            else
                            {
                                if (p->mModel == 5)
                                {
                                    cpu_string = "Pentium II";
                                }
                                else
                                {
                                    cpu_string = "Pentium III";
                                }
                            }
                        }
                    }
                    break;

                    case 6:
                        cpu_string = "Celeron";
                        break;

                    case 8:
                        cpu_string = "Pentium III";
                        break;
                    }
                    break;
                }

                if (signature & 0x1000)
                {
                    cpu_extra_string = " OverDrive";
                }
                else
                if (signature & 0x2000)
                {
                    cpu_extra_string = " dual upgrade";
                }
            }
            else
            if (!strcmp(vendor, "CyrixInstead"))
            {
                vendor_string = "Cyrix";
                switch (p->mFamily)
                {
                case 4:
                    switch (p->mModel)
                    {
                    case 4:
                        cpu_string = "MediaGX";
                        break;
                    }
                    break;

                case 5:
                    switch (p->mModel)
                    {
                    case 2:
                        cpu_string = "6x86";
                        break;

                    case 4:
                        cpu_string = "GXm";
                        break;
                    }
                    break;

                case 6:
                    switch (p->mModel)
                    {
                    case 0:
                        cpu_string = "6x86MX";
                        break;
                    }
                    break;
                }
            }
            else
            if (!strcmp(vendor, "AuthenticAMD"))
            {
                cry_strcpy(p->mVendor, "AMD");
                switch (p->mFamily)
                {
                case 4:
                    cpu_string = "Am486 or Am5x86";
                    break;

                case 5:
                    switch (p->mModel)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        cpu_string = "K5";
                        break;

                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        cpu_string = "K6";
                        break;

                    case 8:
                        cpu_string = "K6-2";
                        break;

                    case 9:
                        cpu_string = "K6-III";
                        break;
                    }
                    break;

                case 6:
                    cpu_string = "Athlon";
                    break;
                }
            }
            else
            if (!strcmp(vendor, "CentaurHauls"))
            {
                vendor_string = "Centaur";
                switch (cpu_type)
                {
                case 5:
                    switch (p->mModel)
                    {
                    case 4:
                        cpu_string = "WinChip";
                        break;

                    case 8:
                        cpu_string = "WinChip2";
                        break;
                    }
                    break;
                }
            }
            else
            if (!strcmp(vendor, "UMC UMC UMC "))
            {
                vendor_string = "UMC";
            }
            else
            if (!strcmp(vendor, "NexGenDriven"))
            {
                vendor_string = "NexGen";
            }
        }
        else
        {
            vendor_string = vendor;
            cpu_string = name;
        }

        if (features_edx & FPU_FLAG)
        {
            fpu_string = "On-Chip";
        }
        else
        {
            fpu_string = "Unknown";
        }
    }

    stack_string sCpuType = stack_string(cpu_string) + cpu_extra_string;

    cry_strcpy(p->mCpuType, sCpuType.c_str());
    cry_strcpy(p->mFpuType, fpu_string);
    cry_strcpy(p->mVendor, vendor_string);

    if (!azstricmp(vendor_string, "Intel"))
    {
        p->meVendor = eCVendor_Intel;
    }
    else
    if (!azstricmp(vendor_string, "Cyrix"))
    {
        p->meVendor = eCVendor_Cyrix;
    }
    else
    if (!azstricmp(vendor_string, "AMD"))
    {
        p->meVendor = eCVendor_AMD;
    }
    else
    if (!azstricmp(vendor_string, "Centaur"))
    {
        p->meVendor = eCVendor_Centaur;
    }
    else
    if (!azstricmp(vendor_string, "NexGen"))
    {
        p->meVendor = eCVendor_NexGen;
    }
    else
    if (!azstricmp(vendor_string, "UMC"))
    {
        p->meVendor = eCVendor_UMC;
    }
    else
    {
        p->meVendor = eCVendor_Unknown;
    }

    if (strstr(cpu_string, "8086"))
    {
        p->meModel = eCpu_8086;
    }
    else
    if (strstr(cpu_string, "80286"))
    {
        p->meModel = eCpu_80286;
    }
    else
    if (strstr(cpu_string, "80386"))
    {
        p->meModel = eCpu_80386;
    }
    else
    if (strstr(cpu_string, "80486"))
    {
        p->meModel = eCpu_80486;
    }
    else
    if (!azstricmp(cpu_string, "Pentium MMX") || !azstricmp(cpu_string, "Pentium"))
    {
        p->meModel = eCpu_Pentium;
    }
    else
    if (!azstricmp(cpu_string, "Pentium Pro"))
    {
        p->meModel = eCpu_PentiumPro;
    }
    else
    if (!azstricmp(cpu_string, "Pentium II"))
    {
        p->meModel = eCpu_Pentium2;
    }
    else
    if (!azstricmp(cpu_string, "Pentium III"))
    {
        p->meModel = eCpu_Pentium3;
    }
    else
    if (!azstricmp(cpu_string, "Pentium 4"))
    {
        p->meModel = eCpu_Pentium4;
    }
    else
    if (!azstricmp(cpu_string, "Celeron"))
    {
        p->meModel = eCpu_Celeron;
    }
    else
    if (!azstricmp(cpu_string, "Pentium II Xeon"))
    {
        p->meModel = eCpu_Pentium2Xeon;
    }
    else
    if (!azstricmp(cpu_string, "Pentium III Xeon"))
    {
        p->meModel = eCpu_Pentium3Xeon;
    }
    else
    if (!azstricmp(cpu_string, "MediaGX"))
    {
        p->meModel = eCpu_CyrixMediaGX;
    }
    else
    if (!azstricmp(cpu_string, "6x86"))
    {
        p->meModel = eCpu_Cyrix6x86;
    }
    else
    if (!azstricmp(cpu_string, "GXm"))
    {
        p->meModel = eCpu_CyrixGXm;
    }
    else
    if (!azstricmp(cpu_string, "6x86MX"))
    {
        p->meModel = eCpu_Cyrix6x86MX;
    }
    else
    if (!azstricmp(cpu_string, "Am486 or Am5x86"))
    {
        p->meModel = eCpu_Am5x86;
    }
    else
    if (!azstricmp(cpu_string, "K5"))
    {
        p->meModel = eCpu_AmK5;
    }
    else
    if (!azstricmp(cpu_string, "K6"))
    {
        p->meModel = eCpu_AmK6;
    }
    else
    if (!azstricmp(cpu_string, "K6-2"))
    {
        p->meModel = eCpu_AmK6_2;
    }
    else
    if (!azstricmp(cpu_string, "K6-III"))
    {
        p->meModel = eCpu_AmK6_3;
    }
    else
    if (!azstricmp(cpu_string, "Athlon"))
    {
        p->meModel = eCpu_AmAthlon;
    }
    else
    if (!azstricmp(cpu_string, "Duron"))
    {
        p->meModel = eCpu_AmDuron;
    }
    else
    if (!azstricmp(cpu_string, "WinChip"))
    {
        p->meModel = eCpu_CenWinChip;
    }
    else
    if (!azstricmp(cpu_string, "WinChip2"))
    {
        p->meModel = eCpu_CenWinChip2;
    }
    else
    {
        p->meModel = eCpu_Unknown;
    }

    p->mbPhysical = true;
    if (!strcmp(vendor_string, "GenuineIntel") && p->mStepping > 4 && HTSupported())
    {
        p->mbPhysical =  (GetAPIC_ID() & LogicalProcPerPhysicalProc() - 1) == 0;
    }

    return 1;
}
#endif //AZ_LEGACY_CRYSYSTEM_TRAIT_DEFINE_DETECT_PROCESSOR

#if defined(MAC) || (defined(LINUX) && !defined(ANDROID))
static void* DetectProcessorThreadProc(void* pData)
{
    DetectProcessor(pData);
    return NULL;
}
#endif // MAC LINUX

// #define SQRT_TEST
#ifdef SQRT_TEST
/* ------------------------------------------------------------------------------ */

ILINE float CorrectInvSqrt(float fNum, float fInvSqrtEst)
{
    // Newton-Rhapson method for improving estimated inv sqrt.
    //  f(x) = x^(-1/2)
    //  f(n) = f(a) + (n-a)f'(a)
    //           = a^(-1/2) + (n-a)(-1/2)a^(-3/2)
    //           = a^(-1/2)*3/2 - na^(-3/2)/2
    //           = e*3/2 - ne^3/2
    return fInvSqrtEst * (1.5f - fNum * fInvSqrtEst * fInvSqrtEst * 0.5f);
}


float Null(float f) { return f; }
float Inv(float f) { return 1.f / f; }

float Square(float f) { return f * f; }
float InvSquare(float f) { return 1.f / (f * f); }

float Sqrt(float f) { return sqrtf(f); }
float SqrtT(float f) { return sqrt_tpl(f); }
float SqrtFT(float f) { return sqrt_fast_tpl(f); }

float InvSqrt(float f) { return 1.f / sqrtf(f); }
float ISqrtT(float f) { return isqrt_tpl(f); }
float ISqrtFT(float f) { return isqrt_fast_tpl(f); }

float SSEInv(float f)
{
    __m128 s = _mm_rcp_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return r;
}
float SSESqrt(float f)
{
    __m128 s = _mm_sqrt_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return r;
}
float SSEISqrt(float f)
{
    __m128 s = _mm_sqrt_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return 1.f / r;
}
float SSERSqrt(float f)
{
    __m128 s = _mm_rsqrt_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return r;
}
float SSERSqrtInv(float f)
{
    __m128 s = _mm_rcp_ss(_mm_rsqrt_ss(_mm_load_ss(&f)));
    float r;
    _mm_store_ss(&r, s);
    return r;
}
float SSERSqrtNR(float f)
{
    __m128 s = _mm_rsqrt_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return CorrectInvSqrt(f, r);
}
float SSERISqrtNR(float f)
{
    __m128 s = _mm_rsqrt_ss(_mm_load_ss(&f));
    float r;
    _mm_store_ss(&r, s);
    return 1.f / CorrectInvSqrt(f, r);
}

inline float cryISqrtf(float fVal)
{
    unsigned int* n1 = (unsigned int*)&fVal;
    unsigned int n = 0x5f3759df - (*n1 >> 1);
    float* n2 = (float*)&n;
    fVal = (1.5f - (fVal * 0.5f) * *n2 * *n2) * *n2;
    return fVal;
}

float cryISqrtNRf(float f)
{
    return CorrectInvSqrt(f, cryISqrtf(f));
}

inline float crySqrtf(float fVal)
{
    return 1.0f / cryISqrtf(fVal);
}

/* ------------------------------------------------------------------------------ */
struct SMathTest
{
    typedef int64 TTime;
    static inline TTime GetTime()
    {
        return CryGetTicks();
    }

    static const int T = 100, N = 1000;
    float fNullTime;

    float aTestVals[T];
    float aResVals[T];

    typedef float (* FFloatFunc)(float f);

    float Timer(const char* sName, FFloatFunc func, FFloatFunc finv)
    {
        for (int i = 0; i < T; i++)
        {
            aResVals[i] = func(aTestVals[i]);
        }
        TTime tStart = GetTime();
        for (int r = 0; r < N; r++)
        {
            for (int i = 0; i < T; i++)
            {
                aResVals[i] = func(aTestVals[i]);
            }
        }
        float fTime = (GetTime() - tStart) / float(N * T);

        // Error computation.
        float fAvgErr = 0.f, fMaxErr = 0.f;
        for (int i = 0; i < T; i++)
        {
            float fErr = abs(finv(aResVals[i]) / aTestVals[i] - 1.f);
            fAvgErr += fErr;
            fMaxErr = max(fMaxErr, fErr);
        }
        fAvgErr /= float(T);

        CryLogAlways("%-20s : %5.2f cycles, avg err %.2e, max err %.2e", sName, fTime - fNullTime, fAvgErr, fMaxErr);

        return fTime;
    };

    SMathTest()
    {
        for (int i = 0; i < T; i++)
        {
            aTestVals[i] = powf(cry_random(1.f, 2.f), cry_random(-30.f, 30.f));
        }

        CryLogAlways("--- Math Test ---");

        fNullTime = 0.f;
        fNullTime = Timer("(null)", &Null, &Null);

        CryLogAlways("-- Inverse methods");
        Timer("1/f", &Inv, &Inv);
        Timer("rcpss", &SSEInv, &Inv);

        CryLogAlways("-- Sqrt methods");
        Timer("sqrtf()", &Sqrt, &Square);
        Timer("sqrt_tpl()", &SqrtT, &Square);
        Timer("sqrt_fast_tpl()", &SqrtFT, &Square);
        Timer("crySqrt()", &crySqrtf, &Square);

        // Timer("sqrtss", &SSESqrt, &Square);
        // Timer("rsqrtss,rcpss", &SSERSqrtInv, &Square);
        Timer("1/rsqrtss,correction", &SSERISqrtNR, &Square);

        CryLogAlways("-- InvSqrt methods");
        Timer("1/sqrtf()", &InvSqrt, &InvSquare);
        Timer("isqrt_tpl()", &ISqrtT, &InvSquare);
        Timer("isqrt_fast_tpl()", &ISqrtFT, &InvSquare);
        Timer("cryISqrt()", &cryISqrtf, &InvSquare);

        Timer("1/sqrtss", &SSEISqrt, &InvSquare);
        // Timer("rsqrtss", &SSERSqrt, &InvSquare);
        // Timer("rsqrtss,correction", &SSERSqrtNR, &InvSquare);
        Timer("cryISqrt,correction", &cryISqrtNRf, &InvSquare);

        CryLogAlways("--------------------");
    }
};

#endif // SQRT_TEST

#if defined(LINUX)
// collection of functions to read from /proc/cpuinfo

static bool proc_read_str(char* buffer, char* output, size_t output_length)
{
    if (!buffer || !output || output_length <= 0)
    {
        return false;
    }
    while (*buffer && *buffer != ':')
    {
        ++buffer;
    }
    if (*buffer == ':')
    {
        buffer += 2;
        cry_strcpy(output, output_length, buffer);
        const int len = strlen(output);
        if (len > 0 && output[len - 1] == '\n')
        {
            output[len - 1] = '\0';
        }
        return true;
    }
    return false;
}


static bool proc_read_int(char* buffer, int& output)
{
    if (!buffer)
    {
        return false;
    }
    while (*buffer && *buffer != ':')
    {
        ++buffer;
    }
    if (*buffer == ':')
    {
        buffer += 2;
        output = atoi(buffer);
        return true;
    }
    return false;
}
#endif

/* ------------------------------------------------------------------------------ */
void CCpuFeatures::Detect(void)
{
    m_NumSystemProcessors = 1;
    m_NumAvailProcessors = 0;

    //////////////////////////////////////////////////////////////////////////
#if AZ_LEGACY_CRYSYSTEM_TRAIT_HASAFFINITYMASK
    CryLogAlways("");

    DWORD_PTR process_affinity_mask = 1;

    /* get the system info to derive the number of processors within the system. */

    SYSTEM_INFO   sys_info;
    DWORD_PTR system_affinity_mask;
    GetSystemInfo(&sys_info);
    m_NumLogicalProcessors = m_NumSystemProcessors = sys_info.dwNumberOfProcessors;
    m_NumAvailProcessors = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &process_affinity_mask, &system_affinity_mask);

    for (unsigned char c = 0; c < m_NumSystemProcessors; c++)
    {
        if (process_affinity_mask & ((DWORD_PTR)1 << c))
        {
            m_NumAvailProcessors++;
            SetProcessAffinityMask(GetCurrentProcess(), DWORD_PTR(1) << c);
            DetectProcessor(&m_Cpu[c]);
            m_Cpu[c].mAffinityMask = ((DWORD_PTR)1 << c);
        }
    }

    SetProcessAffinityMask(GetCurrentProcess(), process_affinity_mask);

    m_bOS_ISSE = false;
    m_bOS_ISSE_EXCEPTIONS = false;
#elif defined(LINUX)
    // Retrieve information from /proc/cpuinfo
    FILE* cpu_info = fopen("/proc/cpuinfo", "r");
    if (!cpu_info)
    {
        m_NumLogicalProcessors = m_NumSystemProcessors = m_NumAvailProcessors = 1;
        CryLogAlways("Could not open /proc/cpuinfo, defaulting values to 1.");
    }
    else
    {
        int nCores = 0;
        int nCpu = -1;
        int index = 0;
        char buffer[512];
        while (!feof(cpu_info))
        {
            if (nCpu >= MAX_CPU)
            {
                --nCpu; //Decrement so the sets after the while loop matches the number of CPUs examined
                CryLogAlways("Found a higher than expected number of CPUs, defaulting to %d", MAX_CPU);
                break;
            }

            fgets(buffer, sizeof(buffer), cpu_info);

            if (buffer[0] == '\0' || buffer[0] == '\n')
            {
                continue;
            }

            if (strncmp("processor", buffer, (index = strlen("processor"))) == 0)
            {
                ++nCpu;
            }
            else if (strncmp("vendor_id", buffer, (index = strlen("vendor_id"))) == 0)
            {
                proc_read_str(&buffer[index], m_Cpu[nCpu].mVendor, sizeof(m_Cpu[nCpu].mVendor));
            }
            else if (strncmp("model name", buffer, (index = strlen("model name"))) == 0)
            {
                proc_read_str(&buffer[index], m_Cpu[nCpu].mCpuType, sizeof(m_Cpu[nCpu].mCpuType));
            }
            else if (strncmp("cpu cores", buffer, (index = strlen("cpu cores"))) == 0 && nCores == 0)
            {
                proc_read_int(&buffer[index], nCores);
            }
            else if (strncmp("fpu", buffer, (index = strlen("fpu"))) == 0)
            {
                while (buffer[index] != ':' && index < 512)
                {
                    ++index;
                }
                if (buffer[index] == ':')
                {
                    if (strncmp(&buffer[index + 2], "yes", 3) == 0)
                    {
                        snprintf(m_Cpu[nCpu].mFpuType, sizeof(m_Cpu[nCpu].mFpuType), "On-Chip");
                    }
                    else
                    {
                        snprintf(m_Cpu[nCpu].mFpuType, sizeof(m_Cpu[nCpu].mFpuType), "Unkown");
                    }
                }
            }
            else if (strncmp("cpu family", buffer, (index = strlen("cpu family"))) == 0)
            {
                proc_read_int(&buffer[index], m_Cpu[nCpu].mFamily);
            }
            else if (strncmp("model", buffer, (index = strlen("model"))) == 0)
            {
                proc_read_int(&buffer[index], m_Cpu[nCpu].mModel);
            }
            else if (strncmp("stepping", buffer, (index = strlen("stepping"))) == 0)
            {
                proc_read_int(&buffer[index], m_Cpu[nCpu].mStepping);
            }
            else if (strncmp("flags", buffer, (index = strlen("flags"))) == 0)
            {
                if (strstr(buffer + index, "mmx"))
                {
                    m_Cpu[nCpu].mFeatures |= CFI_MMX;
                }

                if (strstr(buffer + index, "sse"))
                {
                    m_Cpu[nCpu].mFeatures |= CFI_SSE;
                }

                if (strstr(buffer + index, "sse2"))
                {
                    m_Cpu[nCpu].mFeatures |= CFI_SSE2;
                }
            }
        }
        m_NumLogicalProcessors = m_NumAvailProcessors = nCpu + 1;
        m_NumSystemProcessors = nCores;
    }


#elif defined(APPLE)
    size_t len;
    unsigned int ncpu;

    len = sizeof(ncpu);
    if (sysctlbyname ("hw.physicalcpu_max", &ncpu, &len, NULL, 0) == 0)
    {
        m_NumSystemProcessors = ncpu;
    }
    else
    {
        CryLogAlways("Failed to detect the number of available processors, defaulting to 1");
        m_NumSystemProcessors = 1;
    }

    if (sysctlbyname ("hw.logicalcpu_max", &ncpu, &len, NULL, 0) == 0)
    {
        m_NumAvailProcessors = m_NumLogicalProcessors = ncpu;
    }
    else
    {
        CryLogAlways("Failed to detect the number of available logical processors, defaulting to 1");
        m_NumAvailProcessors = m_NumLogicalProcessors = 1;
    }
    uint64_t cpu_freq;
    len = sizeof(cpu_freq);
    if (sysctlbyname ("hw.cpufrequency_max", &cpu_freq, &len, NULL, 0) != 0)
    {
        CryLogAlways("Failed to detect cpu frequency , defaulting to 0");
        cpu_freq = 0;
    }

    // On macs, the processors are always the same model, so we can easily
    // calculate once and apply the settings for all.
    SCpu cpuInfo;
#if !defined(IOS)
    DetectProcessor(&cpuInfo);
#endif
    for (int c = 0; c < m_NumAvailProcessors; c++)
    {
        m_Cpu[c] = cpuInfo;
    }

#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION CPUDETECT_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(CPUDetect_cpp)
#endif


#if defined(WIN32) || defined(WIN64)
    CryLogAlways("Total number of logical processors: %d", m_NumSystemProcessors);
    CryLogAlways("Number of available logical processors: %d", m_NumAvailProcessors);

    unsigned int numSysCores(1), numProcessCores(1);
    Win32SysInspect::GetNumCPUCores(numSysCores, numProcessCores);
    m_NumSystemProcessors = numSysCores;
    m_NumAvailProcessors = numProcessCores;
    CryLogAlways("Total number of system cores: %d", m_NumSystemProcessors);
    CryLogAlways("Number of cores available to process: %d", m_NumAvailProcessors);

#else
    CryLogAlways("Number of system processors: %d", m_NumSystemProcessors);
    CryLogAlways("Number of available processors: %d", m_NumAvailProcessors);
#endif

    if (m_NumAvailProcessors > MAX_CPU)
    {
        m_NumAvailProcessors = MAX_CPU;
    }

    for (int i = 0; i < m_NumAvailProcessors; i++)
    {
        SCpu* p = &m_Cpu[i];

        CryLogAlways(" ");
        CryLogAlways("Processor %d:", i);
        CryLogAlways("  CPU: %s %s", p->mVendor, p->mCpuType);
        CryLogAlways("  Family: %d, Model: %d, Stepping: %d", p->mFamily, p->mModel, p->mStepping);
        CryLogAlways("  FPU: %s", p->mFpuType);
        CryLogAlways("  3DNow!: %s", (p->mFeatures & CFI_3DNOW) ? "present" : "not present");
        CryLogAlways("  MMX: %s", (p->mFeatures & CFI_MMX) ? "present" : "not present");
        CryLogAlways("  SSE: %s", (p->mFeatures & CFI_SSE) ? "present" : "not present");
        CryLogAlways("  SSE2: %s", (p->mFeatures & CFI_SSE2) ? "present" : "not present");
        CryLogAlways("  SSE3: %s", (p->mFeatures & CFI_SSE3) ? "present" : "not present");
        CryLogAlways("  SSE4.1: %s", (p->mFeatures& CFI_SSE41) ? "present" : "not present");
        if (p->mbSerialPresent)
        {
            CryLogAlways("  Serial number: %s", p->mSerialNumber);
        }
        else
        {
            CryLogAlways("  Serial number not present or disabled");
        }
    }

#ifdef SQRT_TEST
    SMathTest test;
#endif

    CryLogAlways(" ");

    //m_NumPhysicsProcessors = m_NumSystemProcessors;
    for (int i = m_NumPhysicsProcessors = 0; i < m_NumAvailProcessors; i++)
    {
        if (m_Cpu[i].mbPhysical)
        {
            ++m_NumPhysicsProcessors;
        }
    }

    // Set the cpu flags global variable
    g_CpuFlags = 0;
    if (hasMMX())
    {
        g_CpuFlags |= CPUF_MMX;
    }
    if (hasSSE())
    {
        g_CpuFlags |= CPUF_SSE;
    }
    if (hasSSE2())
    {
        g_CpuFlags |= CPUF_SSE2;
    }
    if (hasSSE3())
    {
        g_CpuFlags |= CPUF_SSE3;
    }
    if (hasSSE41())
    {
        g_CpuFlags |= CPUF_SSE41;
    }
    if (has3DNow())
    {
        g_CpuFlags |= CPUF_3DNOW;
    }
    if (hasF16C())
    {
        g_CpuFlags |= CPUF_F16C;
    }
}

