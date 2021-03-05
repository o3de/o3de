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

#ifndef CRYINCLUDE_CRYSYSTEM_CPUDETECT_H
#define CRYINCLUDE_CRYSYSTEM_CPUDETECT_H
#pragma once


//-------------------------------------------------------
/// Cpu class
//-------------------------------------------------------
#if defined(WIN64) || defined(LINUX)
    #define MAX_CPU 96
#else
    #define MAX_CPU 32
#endif

/// Cpu Features
#define CFI_FPUEMULATION 0x01
#define CFI_MMX   0x02
#define CFI_3DNOW 0x04
#define CFI_SSE   0x08
#define CFI_SSE2  0x10
#define CFI_SSE3  0x20
#define CFI_F16C  0x40
#define CFI_SSE41 0x80

/// Type of Cpu Vendor.
enum ECpuVendor
{
    eCVendor_Unknown,
    eCVendor_Intel,
    eCVendor_Cyrix,
    eCVendor_AMD,
    eCVendor_Centaur,
    eCVendor_NexGen,
    eCVendor_UMC,
    eCVendor_M68K
};

/// Type of Cpu Model.
enum ECpuModel
{
    eCpu_Unknown,

    eCpu_8086,
    eCpu_80286,
    eCpu_80386,
    eCpu_80486,
    eCpu_Pentium,
    eCpu_PentiumPro,
    eCpu_Pentium2,
    eCpu_Pentium3,
    eCpu_Pentium4,
    eCpu_Pentium2Xeon,
    eCpu_Pentium3Xeon,
    eCpu_Celeron,
    eCpu_CeleronA,

    eCpu_Am5x86,
    eCpu_AmK5,
    eCpu_AmK6,
    eCpu_AmK6_2,
    eCpu_AmK6_3,
    eCpu_AmK6_3D,
    eCpu_AmAthlon,
    eCpu_AmDuron,

    eCpu_CyrixMediaGX,
    eCpu_Cyrix6x86,
    eCpu_CyrixGXm,
    eCpu_Cyrix6x86MX,

    eCpu_CenWinChip,
    eCpu_CenWinChip2,
};

struct SCpu
{
    ECpuVendor meVendor;
    ECpuModel meModel;
    unsigned long mFeatures;
    bool mbSerialPresent;
    char  mSerialNumber[30];
    int mFamily;
    int mModel;
    int mStepping;
    char mVendor[64];
    char mCpuType[64];
    char mFpuType[64];
    bool mbPhysical; // false for hyperthreaded
    DWORD_PTR mAffinityMask;

    // constructor
    SCpu()
        : meVendor(eCVendor_Unknown)
        , meModel(eCpu_Unknown)
        , mFeatures(0)
        , mbSerialPresent(false)
        , mFamily(0)
        , mModel(0)
        , mStepping(0)
        , mbPhysical(true)
        , mAffinityMask(0)
    {
        memset(mSerialNumber, 0, sizeof(mSerialNumber));
        memset(mVendor, 0, sizeof(mVendor));
        memset(mCpuType, 0, sizeof(mCpuType));
        memset(mFpuType, 0, sizeof(mFpuType));
    }
};

class CCpuFeatures
{
private:
    int m_NumLogicalProcessors;
    int m_NumSystemProcessors;
    int m_NumAvailProcessors;
    int m_NumPhysicsProcessors;
    bool m_bOS_ISSE;
    bool m_bOS_ISSE_EXCEPTIONS;
public:

    SCpu m_Cpu[MAX_CPU];

public:
    CCpuFeatures()
    {
        m_NumLogicalProcessors = 0;
        m_NumSystemProcessors = 0;
        m_NumAvailProcessors  = 0;
        m_NumPhysicsProcessors = 0;
        m_bOS_ISSE = 0;
        m_bOS_ISSE_EXCEPTIONS = 0;
        ZeroMemory(m_Cpu, sizeof(m_Cpu));
    }

    void Detect(void);
    bool hasSSE() { return (m_Cpu[0].mFeatures & CFI_SSE) != 0; }
    bool hasSSE2() { return (m_Cpu[0].mFeatures & CFI_SSE2) != 0; }
    bool hasSSE3() { return (m_Cpu[0].mFeatures & CFI_SSE3) != 0; }
    bool hasSSE41() { return (m_Cpu[0].mFeatures & CFI_SSE41) != 0; }
    bool has3DNow() { return (m_Cpu[0].mFeatures & CFI_3DNOW) != 0; }
    bool hasMMX() { return (m_Cpu[0].mFeatures & CFI_MMX) != 0; }
    bool hasF16C() { return (m_Cpu[0].mFeatures & CFI_F16C) != 0; }

    unsigned int GetLogicalCPUCount() { return m_NumLogicalProcessors; }
    unsigned int GetPhysCPUCount() { return m_NumPhysicsProcessors; }
    unsigned int GetCPUCount() { return m_NumAvailProcessors; }
    DWORD_PTR GetCPUAffinityMask(unsigned int iCPU) { assert(iCPU < MAX_CPU); return iCPU < GetCPUCount() ? m_Cpu[iCPU].mAffinityMask : 0; }
    DWORD_PTR GetPhysCPUAffinityMask(unsigned int iCPU)
    {
        if (iCPU > GetPhysCPUCount())
        {
            return 0;
        }
        int i;
        for (i = 0; (int)iCPU >= 0; i++)
        {
            if (m_Cpu[i].mbPhysical)
            {
                --iCPU;
            }
        }
        PREFAST_ASSUME(i > 0 && i < MAX_CPU);
        return m_Cpu[i - 1].mAffinityMask;
    }
};

#endif // CRYINCLUDE_CRYSYSTEM_CPUDETECT_H
