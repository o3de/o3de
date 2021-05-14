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

#if defined(WIN32) || defined(WIN64)

#include <wbemidl.h>
#include <intrin.h>
#include <d3d9.h>
#include <ddraw.h>
#include <dxgi.h>
#include <d3d11.h>
#include <map>
#include <AzFramework/Archive/IArchive.h>
#include <IConsole.h>
#include <StringUtils.h>

#include "System.h"
#include "AutoDetectSpec.h"

// both function live in CPUDetect.cpp
bool IsAMD();
bool IsIntel();


static void TrimExcessiveWhiteSpaces(char* pStr)
{
    size_t len(strlen(pStr));
    bool remove(true);
    for (size_t i(0); i < len; ++i)
    {
        if (pStr[i] == ' ' && remove)
        {
            size_t newlen(len - 1);

            size_t j(i + 1);
            for (; j < len && pStr[j] == ' '; ++j)
            {
                --newlen;
            }

            size_t ii(i);
            for (; j < len + 1; ++j, ++ii)
            {
                pStr[ii] = pStr[j];
            }

            assert(newlen == strlen(pStr));
            len = newlen;
            remove = false;
        }
        else
        {
            remove = pStr[i] == ' ';
        }
    }

    if (len > 0 && pStr[len - 1] == ' ')
    {
        pStr[len - 1] = '\0';
    }
}


static void GetCPUName(char* pName, size_t bufferSize)
{
    if (!pName || !bufferSize)
    {
        return;
    }

    char name[12 * 4 + 1];

    int CPUInfo[4];
    __cpuid(CPUInfo, 0x80000000);
    if (CPUInfo[0] >= 0x80000004)
    {
        __cpuid(CPUInfo, 0x80000002);
        ((int*)name)[0] = CPUInfo[0];
        ((int*)name)[1] = CPUInfo[1];
        ((int*)name)[2] = CPUInfo[2];
        ((int*)name)[3] = CPUInfo[3];

        __cpuid(CPUInfo, 0x80000003);
        ((int*)name)[4] = CPUInfo[0];
        ((int*)name)[5] = CPUInfo[1];
        ((int*)name)[6] = CPUInfo[2];
        ((int*)name)[7] = CPUInfo[3];

        __cpuid(CPUInfo, 0x80000004);
        ((int*)name)[8] = CPUInfo[0];
        ((int*)name)[9] = CPUInfo[1];
        ((int*)name)[10] = CPUInfo[2];
        ((int*)name)[11] = CPUInfo[3];

        name[48] = '\0';
    }
    else
    {
        name[0] = '\0';
    }

    int ret(azsnprintf(pName, bufferSize, name));
    if (ret >= bufferSize || ret < 0)
    {
        pName[bufferSize - 1] = '\0';
    }
}


void Win32SysInspect::GetOS(SPlatformInfo::EWinVersion& ver, bool& is64Bit, char* pName, size_t bufferSize)
{
    ver = SPlatformInfo::WinUndetected;
    is64Bit = false;

    if (pName && bufferSize)
    {
        pName[0] = '\0';
    }

    //GetVersionEx was changed to work based on how the application is manifest
    //meaning we have to specifically state that this application supports
    //Windows 10, Windows 8.1 etc if we want GetVersionEx to return those version numbers.
    //RtlGetVersion does not require a manifest.
    auto RtlGetVersion = reinterpret_cast<LONG(WINAPI*)(LPOSVERSIONINFOEXW)>(GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion"));
    AZ_Assert(RtlGetVersion, "Failed to get address to RtlGetVersion from ntdll.dll");

    RTL_OSVERSIONINFOEXW sysInfo;
    sysInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    
    if (RtlGetVersion(&sysInfo) == 0)
    {
        if (sysInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (sysInfo.dwMajorVersion == 5)
            {
                if (sysInfo.dwMinorVersion == 0)
                {
                    ver = SPlatformInfo::Win2000;
                }
                else if (sysInfo.dwMinorVersion == 1)
                {
                    ver = SPlatformInfo::WinXP;
                }
                else if (sysInfo.dwMinorVersion == 2)
                {
                    if (sysInfo.wProductType == VER_NT_WORKSTATION)
                    {
                        ver = SPlatformInfo::WinXP; // 64 bit windows actually but this will be detected later anyway
                    }
                    else if (sysInfo.wProductType == VER_NT_SERVER || sysInfo.wProductType == VER_NT_DOMAIN_CONTROLLER)
                    {
                        ver = SPlatformInfo::WinSrv2003;
                    }
                }
            }
            else if (sysInfo.dwMajorVersion == 6)
            {
                if (sysInfo.dwMinorVersion == 0)
                {
                    ver = SPlatformInfo::WinVista;
                }
                else if (sysInfo.dwMinorVersion == 1)
                {
                    ver = SPlatformInfo::Win7;
                }
                else if (sysInfo.dwMinorVersion == 2)
                {
                    ver = SPlatformInfo::Win8;
                }
                else if (sysInfo.dwMinorVersion == 3)
                {
                    ver = SPlatformInfo::Win81;
                }
            }
            else if (sysInfo.dwMajorVersion == 10)
            {
                ver = SPlatformInfo::Win10;
            }
        }
        
        typedef BOOL (WINAPI * FP_GetSystemWow64Directory)(LPSTR, UINT);
        FP_GetSystemWow64Directory pgsw64d((FP_GetSystemWow64Directory) GetProcAddress(GetModuleHandle("kernel32"), "GetSystemWow64DirectoryA"));
        if (pgsw64d)
        {
            char str[MAX_PATH];
            if (!pgsw64d(str, sizeof(str)))
            {
                is64Bit = GetLastError() != ERROR_CALL_NOT_IMPLEMENTED;
            }
            else
            {
                is64Bit = true;
            }
        }

        if (pName && bufferSize)
        {
            const char* windowsVersionText(0);
            switch (ver)
            {
            case SPlatformInfo::Win2000:
                windowsVersionText = "Windows 2000";
                break;
            case SPlatformInfo::WinXP:
                windowsVersionText = "Windows XP";
                break;
            case SPlatformInfo::WinSrv2003:
                windowsVersionText = "Windows Server 2003";
                break;
            case SPlatformInfo::WinVista:
                windowsVersionText = "Windows Vista";
                break;
            case SPlatformInfo::Win7:
                windowsVersionText = "Windows 7";
                break;
            case SPlatformInfo::Win8:
                windowsVersionText = "Windows 8";
                break;
            case SPlatformInfo::Win81:
                windowsVersionText = "Windows 8.1";
                break;
            case SPlatformInfo::Win10:
                windowsVersionText = "Windows 10";
                break;
            default:
                windowsVersionText = "Windows";
                break;
            }

            char sptext[32];
            sptext[0] = '\0';
            if (sysInfo.wServicePackMajor > 0)
            {
                azsnprintf(sptext, sizeof(sptext), "SP %d ", sysInfo.wServicePackMajor);
            }

            int ret(azsnprintf(pName, bufferSize, "%s %s %s(build %d.%d.%d)", windowsVersionText, is64Bit ? "64 bit" : "32 bit",
                    sptext, sysInfo.dwMajorVersion, sysInfo.dwMinorVersion, sysInfo.dwBuildNumber));
            if (ret >= bufferSize || ret < 0)
            {
                pName[bufferSize - 1] = '\0';
            }
        }
    }
}

bool Win32SysInspect::IsVistaKB940105Required()
{
#if defined(WIN32) && !defined(WIN64)
    OSVERSIONINFO osv;
    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);

    if (osv.dwMajorVersion != 6 || osv.dwMinorVersion != 0 || (osv.dwBuildNumber > 6000))
    {
        // This QFE only ever applies to Windows Vista RTM. Windows Vista SP1 already has this fix,
        // and earlier versions of Windows do not implement WDDM
        return false;
    }

    //MEMORYSTATUSEX mex;
    //memset(&mex, 0, sizeof(mex));
    //mex.dwLength = sizeof(mex);
    //GlobalMemoryStatusEx(&mex);

    //if (mex.ullTotalVirtual >= 4294836224)
    //{
    //  // If there is 4 GB of VA space total for this process, then we are a
    //  // 32-bit Large Address Aware application running on a Windows 64-bit OS.

    //  // We could be a 32-bit Large Address Aware application running on a
    //  // Windows 32-bit OS and get up to 3 GB, but that has stability implications.
    //  // Therefore, we recommend the QFE for all 32-bit versions of the OS.

    //  // No need for the fix unless the game is pushing 4 GB of VA
    //  return false;
    //}

    const char* sysFile = "dxgkrnl.sys";

    // Ensure we are checking the system copy of the file
    char sysPath[MAX_PATH];
    GetSystemDirectory(sysPath, sizeof(sysPath));

    cry_strcat(sysPath, "\\drivers\\");
    cry_strcat(sysPath, sysFile);

    char buf[2048];
    if (!GetFileVersionInfo(sysPath, 0, sizeof(buf), buf))
    {
        // This should never happen, but we'll assume it's a newer .sys file since we've
        // narrowed the test to a Windows Vista RTM OS.
        return false;
    }

    VS_FIXEDFILEINFO* ver;
    UINT size;
    if (!VerQueryValue(buf, "\\", (void**) &ver, &size) || size != sizeof(VS_FIXEDFILEINFO) || ver->dwSignature != 0xFEEF04BD)
    {
        // This should never happen, but we'll assume it's a newer .sys file since we've
        // narrowed the test to a Windows Vista RTM OS.
        return false;
    }

    // File major.minor.build.qfe version comparison
    //   WORD major = HIWORD( ver->dwFileVersionMS ); WORD minor = LOWORD( ver->dwFileVersionMS );
    //   WORD build = HIWORD( ver->dwFileVersionLS ); WORD qfe = LOWORD( ver->dwFileVersionLS );

    if (ver->dwFileVersionMS > MAKELONG(0, 6) || (ver->dwFileVersionMS == MAKELONG(0, 6) && ver->dwFileVersionLS >= MAKELONG(20648, 6000)))
    {
        // QFE fix version of dxgkrnl.sys is 6.0.6000.20648
        return false;
    }

    return true;
#else
    return false; // The QFE is not required for a 64-bit native application as it has 8 TB of VA
#endif
}


static void GetSystemMemory(uint64& totSysMem)
{
    typedef BOOL (WINAPI * FP_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
    FP_GlobalMemoryStatusEx pgmsex((FP_GlobalMemoryStatusEx) GetProcAddress(GetModuleHandle("kernel32"), "GlobalMemoryStatusEx"));
    if (pgmsex)
    {
        MEMORYSTATUSEX memStats;
        memStats.dwLength = sizeof(memStats);
        if (pgmsex(&memStats))
        {
            totSysMem = memStats.ullTotalPhys;
        }
        else
        {
            totSysMem = 0;
        }
    }
    else
    {
        MEMORYSTATUS memStats;
        memStats.dwLength = sizeof(memStats);
        GlobalMemoryStatus(&memStats);
        totSysMem = memStats.dwTotalPhys;
    }
}


static bool IsVistaOrAbove()
{
    typedef BOOL (WINAPI * FP_VerifyVersionInfo)(LPOSVERSIONINFOEX, DWORD, DWORDLONG);
    FP_VerifyVersionInfo pvvi((FP_VerifyVersionInfo) GetProcAddress(GetModuleHandle("kernel32"), "VerifyVersionInfoA"));

    if (pvvi)
    {
        typedef ULONGLONG (WINAPI * FP_VerSetConditionMask)(ULONGLONG, DWORD, BYTE);
        FP_VerSetConditionMask pvscm((FP_VerSetConditionMask) GetProcAddress(GetModuleHandle("kernel32"), "VerSetConditionMask"));
        assert(pvscm);

        OSVERSIONINFOEX osvi;
        memset(&osvi, 0, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osvi.dwMajorVersion = 6;
        osvi.dwMinorVersion = 0;
        osvi.wServicePackMajor = 0;
        osvi.wServicePackMinor = 0;

        ULONGLONG mask(0);
        mask = pvscm(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        mask = pvscm(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
        mask = pvscm(mask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        mask = pvscm(mask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

        if (pvvi(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR, mask))
        {
            return true;
        }
    }

    return false;
}


// Preferred solution to determine the number of available CPU cores, works reliably only on WinVista/Win7 32/64 and above
// See http://msdn2.microsoft.com/en-us/library/ms686694.aspx for reasons
static void GetNumCPUCoresGlpi(unsigned int& totAvailToSystem, unsigned int& totAvailToProcess)
{
    typedef BOOL (WINAPI * FP_GetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
    FP_GetLogicalProcessorInformation pglpi((FP_GetLogicalProcessorInformation) GetProcAddress(GetModuleHandle("kernel32"), "GetLogicalProcessorInformation"));
    if (pglpi && IsVistaOrAbove())
    {
        unsigned long bufferSize(0);
        pglpi(0, &bufferSize);

        void* pBuffer(alloca(bufferSize));

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pLogProcInfo((SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) pBuffer);
        if (pLogProcInfo && pglpi(pLogProcInfo, &bufferSize))
        {
            DWORD_PTR processAffinity, systemAffinity;
            GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity);

            unsigned long numEntries(bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
            for (unsigned long i(0); i < numEntries; ++i)
            {
                switch (pLogProcInfo[i].Relationship)
                {
                case RelationProcessorCore:
                {
                    ++totAvailToSystem;
                    if (pLogProcInfo[i].ProcessorMask & processAffinity)
                    {
                        ++totAvailToProcess;
                    }
                }
                break;

                default:
                    break;
                }
            }
        }
    }
}


class CApicExtractor
{
public:
    CApicExtractor(unsigned int logProcsPerPkg = 1, unsigned int coresPerPkg = 1)
    {
        SetPackageTopology(logProcsPerPkg, coresPerPkg);
    }

    unsigned char SmtId(unsigned char apicId) const
    {
        return apicId & m_smtIdMask.mask;
    }

    unsigned char CoreId(unsigned char apicId) const
    {
        return (apicId & m_coreIdMask.mask) >> m_smtIdMask.width;
    }

    unsigned char PackageId(unsigned char apicId) const
    {
        return (apicId & m_pkgIdMask.mask) >> (m_smtIdMask.width + m_coreIdMask.width);
    }

    unsigned char PackageCoreId(unsigned char apicId) const
    {
        return (apicId & (m_pkgIdMask.mask | m_coreIdMask.mask)) >> m_smtIdMask.width;
    }

    unsigned int GetLogProcsPerPkg() const
    {
        return m_logProcsPerPkg;
    }

    unsigned int GetCoresPerPkg() const
    {
        return m_coresPerPkg;
    }

    void SetPackageTopology(unsigned int logProcsPerPkg, unsigned int coresPerPkg)
    {
        m_logProcsPerPkg   = (unsigned char) logProcsPerPkg;
        m_coresPerPkg      = (unsigned char) coresPerPkg;

        m_smtIdMask.width   = GetMaskWidth(m_logProcsPerPkg / m_coresPerPkg);
        m_coreIdMask.width  = GetMaskWidth(m_coresPerPkg);
        m_pkgIdMask.width   = 8 - (m_smtIdMask.width + m_coreIdMask.width);

        m_pkgIdMask.mask    = (unsigned char) (0xFF << (m_smtIdMask.width + m_coreIdMask.width));
        m_coreIdMask.mask   = (unsigned char) ((0xFF << m_smtIdMask.width) ^ m_pkgIdMask.mask);
        m_smtIdMask.mask    = (unsigned char) ~(0xFF << m_smtIdMask.width);
    }

private:
    unsigned char GetMaskWidth(unsigned char maxIds) const
    {
        --maxIds;
        unsigned char msbIdx(8);
        unsigned char msbMask(0x80);
        while (msbMask && !(msbMask & maxIds))
        {
            --msbIdx;
            msbMask >>= 1;
        }
        return msbIdx;
    }

    struct IdMask
    {
        unsigned char width;
        unsigned char mask;
    };

    unsigned char m_logProcsPerPkg;
    unsigned char m_coresPerPkg;
    IdMask m_smtIdMask;
    IdMask m_coreIdMask;
    IdMask m_pkgIdMask;
};


// Fallback solution for WinXP 32/64
static void GetNumCPUCoresApic(unsigned int& totAvailToSystem, unsigned int& totAvailToProcess)
{
    unsigned int numLogicalPerPhysical(1);
    unsigned int numCoresPerPhysical(1);

    int CPUInfo[4];
    __cpuid(CPUInfo, 0x00000001);
    if ((CPUInfo[3] & 0x10000000) != 0) // Hyperthreading / Multicore bit set
    {
        numLogicalPerPhysical = (CPUInfo[1] & 0x00FF0000) >> 16;

        if (IsIntel())
        {
            __cpuid(CPUInfo, 0x00000000);
            if (CPUInfo[0] >= 0x00000004)
            {
                __cpuidex(CPUInfo, 4, 0);
                numCoresPerPhysical = ((CPUInfo[0] & 0xFC000000) >> 26) + 1;
            }
        }
        else if (IsAMD())
        {
            __cpuid(CPUInfo, 0x80000000);
            if (CPUInfo[0] >= 0x80000008)
            {
                __cpuid(CPUInfo, 0x80000008);
                if (CPUInfo[2] & 0x0000F000)
                {
                    numCoresPerPhysical = 1 << ((CPUInfo[2] & 0x0000F000) >> 12);
                }
                else
                {
                    numCoresPerPhysical = (CPUInfo[2] & 0xFF) + 1;
                }
            }
        }
    }

    HANDLE hCurProcess(GetCurrentProcess());
    HANDLE hCurThread(GetCurrentThread());

    const int c_maxLogicalProcessors(sizeof(DWORD_PTR) * 8);
    unsigned char apicIds[c_maxLogicalProcessors] = { 0 };
    unsigned char items(0);

    DWORD_PTR processAffinity, systemAffinity;
    GetProcessAffinityMask(hCurProcess, &processAffinity, &systemAffinity);

    if (systemAffinity == 1)
    {
        assert(numLogicalPerPhysical == 1);
        apicIds[items++] = 0;
    }
    else
    {
        if (processAffinity != systemAffinity)
        {
            SetProcessAffinityMask(hCurProcess, systemAffinity);
        }

        DWORD_PTR prevThreadAffinity(0);
        for (DWORD_PTR threadAffinity = 1; threadAffinity && threadAffinity <= systemAffinity; threadAffinity <<= 1)
        {
            if (systemAffinity & threadAffinity)
            {
                if (!prevThreadAffinity)
                {
                    assert(!items);
                    prevThreadAffinity = SetThreadAffinityMask(hCurThread, threadAffinity);
                }
                else
                {
                    assert(items > 0);
                    SetThreadAffinityMask(hCurThread, threadAffinity);
                }

                Sleep(0);

                int CPUInfo2[4];
                __cpuid(CPUInfo2, 0x00000001);
                apicIds[items++] = (unsigned char) ((CPUInfo2[1] & 0xFF000000) >> 24);
            }
        }

        SetProcessAffinityMask(hCurProcess, processAffinity);
        SetThreadAffinityMask(hCurThread, prevThreadAffinity);
        Sleep(0);
    }

    CApicExtractor apicExtractor(numLogicalPerPhysical, numCoresPerPhysical);

    totAvailToSystem = 0;
    {
        unsigned char pkgCoreIds[c_maxLogicalProcessors] = { 0 };
        for (unsigned int i(0); i < items; ++i)
        {
            unsigned int j(0);
            for (; j < totAvailToSystem; ++j)
            {
                if (pkgCoreIds[j] == apicExtractor.PackageCoreId(apicIds[i]))
                {
                    break;
                }
            }
            if (j == totAvailToSystem)
            {
                pkgCoreIds[j] = apicExtractor.PackageCoreId(apicIds[i]);
                ++totAvailToSystem;
            }
        }
    }

    totAvailToProcess = 0;
    {
        unsigned char pkgCoreIds[c_maxLogicalProcessors] = { 0 };
        for (unsigned int i(0); i < items; ++i)
        {
            if (processAffinity & ((DWORD_PTR) 1 << i))
            {
                unsigned int j(0);
                for (; j < totAvailToProcess; ++j)
                {
                    if (pkgCoreIds[j] == apicExtractor.PackageCoreId(apicIds[i]))
                    {
                        break;
                    }
                }
                if (j == totAvailToProcess)
                {
                    pkgCoreIds[j] = apicExtractor.PackageCoreId(apicIds[i]);
                    ++totAvailToProcess;
                }
            }
        }
    }
}

const char* Win32SysInspect::GetFeatureLevelAsString(Win32SysInspect::DXFeatureLevel featureLevel)
{
    switch (featureLevel)
    {
    case Win32SysInspect::DXFL_Undefined:
        return "unknown";
    case Win32SysInspect::DXFL_9_1:
        return "DX9 (SM 2.0)";
    case Win32SysInspect::DXFL_9_2:
        return "DX9 (SM 2.0)";
    case Win32SysInspect::DXFL_9_3:
        return "DX9 (SM 2.x)";
    case Win32SysInspect::DXFL_10_0:
        return "DX10 (SM 4.0)";
    case Win32SysInspect::DXFL_10_1:
        return "DX10.1 (SM 4.x)";
    case Win32SysInspect::DXFL_11_0:
    default:
        return "DX11 (SM 5.0)";
    }
}

void Win32SysInspect::GetNumCPUCores(unsigned int& totAvailToSystem, unsigned int& totAvailToProcess)
{
    totAvailToSystem = 0;
    totAvailToProcess = 0;

    GetNumCPUCoresGlpi(totAvailToSystem, totAvailToProcess);

    if (!totAvailToSystem)
    {
        GetNumCPUCoresApic(totAvailToSystem, totAvailToProcess);
    }
}


static Win32SysInspect::DXFeatureLevel GetFeatureLevel(D3D_FEATURE_LEVEL featureLevel)
{
    switch (featureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1:
        return Win32SysInspect::DXFL_9_1;
    case D3D_FEATURE_LEVEL_9_2:
        return Win32SysInspect::DXFL_9_2;
    case D3D_FEATURE_LEVEL_9_3:
        return Win32SysInspect::DXFL_9_3;
    case D3D_FEATURE_LEVEL_10_0:
        return Win32SysInspect::DXFL_10_0;
    case D3D_FEATURE_LEVEL_10_1:
        return Win32SysInspect::DXFL_10_1;
    case D3D_FEATURE_LEVEL_11_0:
    default:
        return Win32SysInspect::DXFL_11_0;
    }
}

static bool FindGPU(DXGI_ADAPTER_DESC1& adapterDesc, Win32SysInspect::DXFeatureLevel& featureLevel)
{
    memset(&adapterDesc, 0, sizeof(adapterDesc));
    featureLevel = Win32SysInspect::DXFL_Undefined;

    if (!IsVistaOrAbove())
    {
        return false;
    }

    typedef HRESULT (WINAPI * FP_CreateDXGIFactory1)(REFIID, void**);
    FP_CreateDXGIFactory1 pCDXGIF = (FP_CreateDXGIFactory1) GetProcAddress(LoadLibraryA("dxgi.dll"), "CreateDXGIFactory1");

    IDXGIFactory1* pFactory = 0;
    if (pCDXGIF && SUCCEEDED(pCDXGIF(__uuidof(IDXGIFactory1), (void**) &pFactory)) && pFactory)
    {
        typedef HRESULT (WINAPI * FP_D3D11CreateDevice)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, CONST D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
        FP_D3D11CreateDevice pD3D11CD = (FP_D3D11CreateDevice) GetProcAddress(LoadLibraryA("d3d11.dll"), "D3D11CreateDevice");

        if (pD3D11CD)
        {
            unsigned int nAdapter = 0;
            IDXGIAdapter1* pAdapter = 0;
            while (pFactory->EnumAdapters1(nAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                if (pAdapter)
                {
                    ID3D11Device* pDevice = 0;
                    D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1};
                    D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_9_1;
                    HRESULT hr = pD3D11CD(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, levels, sizeof(levels) / sizeof(levels[0]), D3D11_SDK_VERSION, &pDevice, &deviceFeatureLevel, NULL);
                    if (SUCCEEDED(hr) && pDevice)
                    {
                        IDXGIOutput* pOutput = 0;
                        const bool displaysConnected = SUCCEEDED(pAdapter->EnumOutputs(0, &pOutput)) && pOutput;
                        SAFE_RELEASE(pOutput);

                        DXGI_ADAPTER_DESC1 ad;
                        pAdapter->GetDesc1(&ad);

                        const Win32SysInspect::DXFeatureLevel fl = GetFeatureLevel(deviceFeatureLevel);

                        if (featureLevel < fl && displaysConnected)
                        {
                            adapterDesc = ad;
                            featureLevel = fl;
                        }
                    }

                    SAFE_RELEASE(pDevice);
                    SAFE_RELEASE(pAdapter);
                }
                ++nAdapter;
            }
        }
    }
    SAFE_RELEASE(pFactory);
    return featureLevel != Win32SysInspect::DXFL_Undefined;
}


bool Win32SysInspect::IsDX11Supported()
{
    DXGI_ADAPTER_DESC1 adapterDesc = {};
    DXFeatureLevel featureLevel = Win32SysInspect::DXFL_Undefined;
    return FindGPU(adapterDesc, featureLevel) && featureLevel >= DXFL_11_0;
}


bool Win32SysInspect::GetGPUInfo(char* pName, size_t bufferSize, unsigned int& vendorID, unsigned int& deviceID, unsigned int& totLocalVidMem, DXFeatureLevel& featureLevel)
{
    if (pName && bufferSize)
    {
        pName[0] = '\0';
    }

    vendorID = 0;
    deviceID = 0;
    totLocalVidMem = 0;
    featureLevel = Win32SysInspect::DXFL_Undefined;

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    const bool gpuFound = FindGPU(adapterDesc, featureLevel);
    if (gpuFound)
    {
        vendorID = adapterDesc.VendorId;
        deviceID = adapterDesc.DeviceId;

        if (pName && bufferSize)
        {
            sprintf_s(pName, bufferSize, "%s", CryStringUtils::WStrToUTF8(adapterDesc.Description).c_str());
        }

        totLocalVidMem = adapterDesc.DedicatedVideoMemory;
    }

    return gpuFound;
}


class CGPURating
{
public:
    CGPURating();
    ~CGPURating();

    int GetRating(unsigned int vendorId, unsigned int deviceId) const;

private:
    struct SGPUID
    {
        SGPUID(unsigned int vendorId, unsigned int deviceId)
            : vendor(vendorId)
            , device(deviceId)
        {
        }

        bool operator < (const SGPUID& rhs) const
        {
            if (vendor == rhs.vendor)
            {
                return device < rhs.device;
            }
            else
            {
                return vendor < rhs.vendor;
            }
        }

        unsigned int vendor;
        unsigned int device;
    };

    typedef std::map<SGPUID, int> GPURatingMap;

private:
    GPURatingMap m_gpuRatingMap;
};


static size_t SafeReadLine(AZ::IO::IArchive* pPak, AZ::IO::HandleType fileHandle, char* buffer, size_t bufferSize)
{
    assert(buffer && bufferSize);

    memset(buffer, 0, bufferSize);

    size_t bytesRead = pPak->FRead(buffer, bufferSize - 1, fileHandle);
    if (!bytesRead)
    {
        return 0;
    }

    char* currentPosition = buffer;
    size_t len = 0;

    bool done = false;
    int slashRPosition = -1;
    do
    {
        if (*currentPosition != '\r' && *currentPosition != '\n' && len < bufferSize - 1)
        {
            len++;
            currentPosition++;
        }
        else
        {
            done = true;
            if (*currentPosition == '\r')
            {
                slashRPosition = len;
            }
        }
    } while (!done);

    // null terminate string
    buffer[len] = '\0';

    //////////////////////////////////////////
    //seek back to the end of the string
    int seekback = bytesRead - len - 1;

    // handle CR/LF for file coming from different platforms
    if (slashRPosition > -1 && bytesRead > slashRPosition && buffer[slashRPosition + 1] == '\n')
    {
        seekback--;
    }
    pPak->FSeek(fileHandle, -seekback, SEEK_CUR);
    ///////////////////////////////////////////

    return len;
}


#define BUILDPATH_GPURATING(x) "config/gpu/" x

CGPURating::CGPURating()
{
    auto  pPak(gEnv->pCryPak);

    AZ::IO::ArchiveFileIterator h(pPak->FindFirst(BUILDPATH_GPURATING("*.txt")));
    if (h)
    {
        do
        {
            char filename[128];
            azsnprintf(filename, sizeof(filename), BUILDPATH_GPURATING("%.*s"), aznumeric_cast<int>(h.m_filename.size()), h.m_filename.data());

            AZ::IO::HandleType fileHandle = pPak->FOpen(filename, "rb");
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                size_t lineNr(0);
                while (!pPak->FEof(fileHandle))
                {
                    char line[1024];
                    line[0] = '\0';
                    size_t len(SafeReadLine(pPak, fileHandle, line, sizeof(line)));
                    ++lineNr;

                    if (len > 2 && line[0] != '/' && line[1] != '/')
                    {
                        unsigned int vendorId(0), deviceId(0);
                        int rating(0);
                        if (_snscanf_s(line, sizeof(line), "%x,%x,%d", &vendorId, &deviceId, &rating) == 3)
                        {
                            GPURatingMap::iterator it(m_gpuRatingMap.find(SGPUID(vendorId, deviceId)));
                            if (it == m_gpuRatingMap.end())
                            {
                                m_gpuRatingMap.insert(GPURatingMap::value_type(SGPUID(vendorId, deviceId), rating));
                            }
                            else
                            {
                                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                                    "%s line %d contains a multiple defined GPU rating!", filename, lineNr);
                            }
                        }
                        else
                        {
                            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                                "%s line %d contains incomplete GPU rating!", filename, lineNr);
                        }
                    }
                }

                pPak->FClose(fileHandle);
            }
        } while (h = pPak->FindNext(h));

        pPak->FindClose(h);
    }
}


CGPURating::~CGPURating()
{
}


int CGPURating::GetRating(unsigned int vendorId, unsigned int deviceId) const
{
    GPURatingMap::const_iterator it(m_gpuRatingMap.find(SGPUID(vendorId, deviceId)));
    if (it != m_gpuRatingMap.end())
    {
        return (*it).second;
    }
    else
    {
        return 0;
    }
}


int Win32SysInspect::GetGPURating([[maybe_unused]] unsigned int vendorId, [[maybe_unused]] unsigned int deviceId)
{
    return 0; // All GPUs unrated as the database is out of date

    //CGPURating gpuRatingDb;
    //return gpuRatingDb.GetRating(vendorId, deviceId);
}


static int GetFinalSpecValue(int cpuRating, unsigned int totSysMemMB, int gpuRating, unsigned int totVidMemMB, ESystemConfigSpec maxConfigSpec)
{
    int sysMemRating = 1;
    if (totSysMemMB >= Win32SysInspect::SafeMemoryThreshold(12228))
    {
        sysMemRating = 3;
    }
    else if (totSysMemMB >= Win32SysInspect::SafeMemoryThreshold(8192))
    {
        sysMemRating = 2;
    }

    cpuRating = sysMemRating < cpuRating ? sysMemRating : cpuRating;

    // just a sanity check, GPU should reflect overall GPU perf including memory (higher rated GPUs usually come with enough memory)
    if (totVidMemMB < Win32SysInspect::SafeMemoryThreshold(1024))
    {
        gpuRating = 1;
    }

    int finalRating = cpuRating < gpuRating ? cpuRating : gpuRating;

    return min(finalRating, (int) maxConfigSpec);
}


void CSystem::AutoDetectSpec(const bool detectResolution)
{
    CryLogAlways("Running machine spec auto detect (%d bit)...", sizeof(void*) << 3);

    char tempBuf[512];

    // get OS
    SPlatformInfo::EWinVersion winVer(SPlatformInfo::WinUndetected);
    bool is64bit(false);
    Win32SysInspect::GetOS(winVer, is64bit, tempBuf, sizeof(tempBuf));
    CryLogAlways("- %s", tempBuf);

    // get system memory
    uint64 totSysMem(0);
    GetSystemMemory(totSysMem);
    CryLogAlways("- System memory");
    CryLogAlways("--- %d MB", totSysMem >> 20);

    // get CPU name
    GetCPUName(tempBuf, sizeof(tempBuf));
    TrimExcessiveWhiteSpaces(tempBuf);
    CryLogAlways("- %s", tempBuf);

    // get number of CPU cores
    unsigned int numSysCores(1), numProcCores(1);
    Win32SysInspect::GetNumCPUCores(numSysCores, numProcCores);
    CryLogAlways("--- Number of available cores: %d (out of %d)", numProcCores, numSysCores);

    // get CPU rating
    const int cpuRating = numProcCores >= 4 ? 3 : (numProcCores >= 3 ? 2 : 1);

    // get GPU info
    unsigned int gpuVendorId(0), gpuDeviceId(0), totVidMem(0);
    Win32SysInspect::DXFeatureLevel featureLevel(Win32SysInspect::DXFL_Undefined);
    Win32SysInspect::GetGPUInfo(tempBuf, sizeof(tempBuf), gpuVendorId, gpuDeviceId, totVidMem, featureLevel);

    CryLogAlways("- %s (vendor = 0x%.4x, device = 0x%.4x)", tempBuf, gpuVendorId, gpuDeviceId);
    CryLogAlways("--- Dedicated video memory: %d MB", totVidMem >> 20);
    CryLogAlways("--- Feature level: %s", GetFeatureLevelAsString(featureLevel));

    // get GPU rating
    const int gpuRating = (totVidMem >> 20) >= Win32SysInspect::SafeMemoryThreshold(4096) ? 3 : ((totVidMem >> 20) >= Win32SysInspect::SafeMemoryThreshold(2048) ? 2 : 1);

    // get final rating
    int finalSpecValue(GetFinalSpecValue(cpuRating, totSysMem >> 20, gpuRating, totVidMem >> 20, CONFIG_VERYHIGH_SPEC));
    CryLogAlways("- Final rating: Machine class %d", finalSpecValue);

    m_sys_GraphicsQuality->Set(finalSpecValue);

    if (detectResolution)
    {
        if ((m_rWidth->GetFlags() & VF_WASINCONFIG) == 0)
        {
            m_rWidth->Set(GetSystemMetrics(SM_CXFULLSCREEN));
        }
        if ((m_rHeight->GetFlags() & VF_WASINCONFIG) == 0)
        {
            m_rHeight->Set(GetSystemMetrics(SM_CYFULLSCREEN));
        }
        if ((m_rFullscreen->GetFlags() & VF_WASINCONFIG) == 0)
        {
            m_rFullscreen->Set(1);
        }
    }
}


#else

#include "System.h"

void CSystem::AutoDetectSpec(const bool detectResolution)
{
    AZ_UNUSED(detectResolution);
}


#endif
