/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#if defined(USE_NSIGHT_AFTERMATH)   // To enable nsight aftermath, download and install Nsight AfterMath and add 'ATOM_AFTERMATH_PATH=%path_to_the_install_folder%' to environment variables (or alternatively set LY_AFTERMATH_PATH in CMake options)
GpuCrashTracker::~GpuCrashTracker()
{
    // If initialized, disable GPU crash dumps
    if (m_initialized)
    {
        GFSDK_Aftermath_DisableGpuCrashDumps();
    }
}

void GpuCrashTracker::EnableGPUCrashDumps()
{
    // Enable GPU crash dumps and set up the callbacks for crash dump notifications,
    // shader debug information notifications, and providing additional crash
    // dump description data.Only the crash dump callback is mandatory. The other two
    // callbacks are optional and can be omitted, by passing nullptr, if the corresponding
    // functionality is not used.
    // The DeferDebugInfoCallbacks flag enables caching of shader debug information data
    // in memory. If the flag is set, ShaderDebugInfoCallback will be called only
    // in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
    // ShaderDebugInfoCallback will be called for every shader that is compiled.
    GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
        GFSDK_Aftermath_Version_API,
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, // Let the Nsight Aftermath library cache shader debug information.
        GpuCrashDumpCallback,                                             // Register callback for GPU crash dumps.
        ShaderDebugInfoCallback,                                          // Register callback for shader debug information.
        CrashDumpDescriptionCallback,                                     // Register callback for GPU crash dump description.
        ResolveMarkerCallback,                                            // Register callback for mark resolution
        this);                                                           // Set the GpuCrashTracker object as user data for the above callbacks.
    
    m_initialized = GFSDK_Aftermath_SUCCEED(result);

    char executableFolder[AZ::IO::MaxPathLength];
    AZ::Utils::GetExecutableDirectory(executableFolder, AZ::IO::MaxPathLength);
    m_executableFolder = executableFolder;
    m_projectName = AZ::Utils::GetProjectName();
}

void GpuCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
    if (!m_initialized)
    {
        return;
    }

    // Make sure only one thread at a time...
    AZStd::unique_lock<AZStd::mutex> lock(m_mutex);
    // Write to file for later in-depth analysis with Nsight Graphics.
    WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

void GpuCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
    if (!m_initialized)
    {
        return;
    }

    // Get shader debug information identifier
    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
    if (GFSDK_Aftermath_SUCCEED(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
        GFSDK_Aftermath_Version_API,
        pShaderDebugInfo,
        shaderDebugInfoSize,
        &identifier)))
    {
        // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
        WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
    }
    else
    {
        AZ_TracePrintf("Aftermath", "Failed to get shader debug info\n");
    }
}

void GpuCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
    if (!m_initialized)
    {
        return;
    }

    // Add some basic description about the crash. This is called after the GPU crash happens, but before
    // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
    // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, m_projectName.c_str());
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "GPU crash related dump for nv aftermath");
}

void GpuCrashTracker::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
    if (!m_initialized)
    {
        return;
    }
    // Create a GPU crash dump decoder object for the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
    GFSDK_Aftermath_Result result = GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
        GFSDK_Aftermath_Version_API,
        pGpuCrashDump,
        gpuCrashDumpSize,
        &decoder);
    AssertOnError(result);

    // Use the decoder object to read basic information, like application
    // name, PID, etc. from the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
    result = GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo);
    AssertOnError(result);

     // Create a unique file name for writing the crash dump data to a file.
    // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
    // driver release) we may see redundant crash dumps. As a workaround,
    // attach a unique count to each generated file name.
    static int count = 0;
    const AZStd::string baseFileName = AZStd::string::format("%s/%s-%i-%i", m_executableFolder.c_str(), m_projectName.c_str(), baseInfo.pid, ++count);

    const AZStd::string crashDumpFileName = baseFileName + ".nv-gpudmp";
    AZ::IO::SystemFile dumpFile;
    dumpFile.Open(crashDumpFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
    dumpFile.Write(static_cast<const char*>(pGpuCrashDump), gpuCrashDumpSize);
    dumpFile.Close();

    // Decode the crash dump to a JSON string.
    // Step 1: Generate the JSON and get the size.
    uint32_t jsonSize = 0;
    result = GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
        GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
        ShaderDebugInfoLookupCallback,
        ShaderLookupCallback,
        ShaderSourceDebugInfoLookupCallback,
        this,
        &jsonSize);
    AssertOnError(result);

    // Step 2: Allocate a buffer and fetch the generated JSON.
    AZStd::unique_ptr<char[]> json = AZStd::make_unique<char[]>(jsonSize);
    result = GFSDK_Aftermath_GpuCrashDump_GetJSON(
        decoder,
        jsonSize,
        json.get());
    AssertOnError(result);

    // Write the the crash dump data as JSON to a file.
    const AZStd::string jsonFileName = crashDumpFileName + ".json";
    AZ::IO::SystemFile jsonFile;
    jsonFile.Open(jsonFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
    jsonFile.Write(json.get(), jsonSize - 1);
    jsonFile.Close();

    // Destroy the GPU crash dump decoder object.
    result = GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder);
    AssertOnError(result);
}

void GpuCrashTracker::WriteShaderDebugInformationToFile(
    GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
    const void* pShaderDebugInfo,
    const uint32_t shaderDebugInfoSize)
{
    if (!m_initialized)
    {
        return;
    }
     // Create an unique file name from identifier
    const AZStd::string fileName = AZStd::string::format("%s/%016llX-%016llX.nvdbg", m_executableFolder.c_str(), identifier.id[0], identifier.id[1]);
    
    AZ::IO::SystemFile shaderDebugFile;
    shaderDebugFile.Open(fileName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
    shaderDebugFile.Write(pShaderDebugInfo, shaderDebugInfoSize);
}

void GpuCrashTracker::OnShaderDebugInfoLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::OnShaderLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderBinaryHash& shaderHash,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderBinary) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::OnResolveMarker(
    [[maybe_unused]] const void* pMarker, [[maybe_unused]] const uint32_t markerDataSize,[[maybe_unused]] void** resolvedMarkerData, [[maybe_unused]] uint32_t* markerSize) const
{
}

void GpuCrashTracker::OnShaderSourceDebugInfoLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderBinary) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::GpuCrashDumpCallback(
    const void* pGpuCrashDump,
    const uint32_t gpuCrashDumpSize,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

void GpuCrashTracker::ShaderDebugInfoCallback(
    const void* pShaderDebugInfo,
    const uint32_t shaderDebugInfoSize,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

void GpuCrashTracker::CrashDumpDescriptionCallback(
    PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnDescription(addDescription);
}

void GpuCrashTracker::ShaderDebugInfoLookupCallback(
    const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
    PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderDebugInfoLookup(*pIdentifier, setShaderDebugInfo);
}

void GpuCrashTracker::ShaderLookupCallback(
    const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderLookup(*pShaderHash, setShaderBinary);
}

void GpuCrashTracker::ResolveMarkerCallback(
    const void* pMarker,
    const uint32_t markerDataSize,
    void* pUserData,
    void** resolvedMarkerData,
    uint32_t* markerSize)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnResolveMarker(pMarker, markerDataSize, resolvedMarkerData, markerSize);
}

void GpuCrashTracker::ShaderSourceDebugInfoLookupCallback(
    const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderSourceDebugInfoLookup(*pShaderDebugName, setShaderBinary);
}
#endif

