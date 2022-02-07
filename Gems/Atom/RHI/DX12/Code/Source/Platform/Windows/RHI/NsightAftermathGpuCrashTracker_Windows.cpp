/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Debug/EventTrace.h>

#if defined(USE_NSIGHT_AFTERMATH)
GpuCrashTracker::~GpuCrashTracker()
{
    // If initialized, disable GPU crash dumps
    if (m_initialized)
    {
        GFSDK_Aftermath_DisableGpuCrashDumps();
    }
}

void GpuCrashTracker::AddContext(GFSDK_Aftermath_ContextHandle cntxHndl)
{
    m_contextHandles.push_back(cntxHndl);
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
        GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
        GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, // Let the Nsight Aftermath library cache shader debug information.
        GpuCrashDumpCallback,                                             // Register callback for GPU crash dumps.
        ShaderDebugInfoCallback,                                          // Register callback for shader debug information.
        CrashDumpDescriptionCallback,                                     // Register callback for GPU crash dump description.
        this);                                                           // Set the GpuCrashTracker object as user data for the above callbacks.
    
    m_initialized = GFSDK_Aftermath_SUCCEED(result);
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

void GpuCrashTracker::OnShaderDebugInfo([[maybe_unused]] const void* pShaderDebugInfo, [[maybe_unused]] const uint32_t shaderDebugInfoSize)
{
    AZ_UNUSED(pShaderDebugInfo);
    AZ_UNUSED(shaderDebugInfoSize);

    if (!m_initialized)
    {
        return;
    }
}

void GpuCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
    if (!m_initialized)
    {
        return;
    }

    char executableFolder[AZ::IO::MaxPathLength];
    // Add some basic description about the crash. This is called after the GPU crash happens, but before
    // the actual GPU crash dump callback. The provided data is included in the crash dump and can be
    // retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
    AZ::Utils::GetExecutableDirectory(executableFolder, AZ::IO::MaxPathLength);
    AZ::IO::FixedMaxPath fileAbsolutePath{executableFolder};
    fileAbsolutePath /= AZ::Utils::GetProjectName();
    addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, fileAbsolutePath.c_str());

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

    // Use the decoder object to query the application name that was set
    // in the GPU crash dump description.
    uint32_t applicationNameLength = 0;
    result = GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
        &applicationNameLength);
    AssertOnError(result);

    AZStd::vector<char> applicationName(applicationNameLength, '\0');

    result = GFSDK_Aftermath_GpuCrashDump_GetDescription(
        decoder,
        GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
        uint32_t(applicationName.size()),
        applicationName.data());
    AssertOnError(result);

    // Create a unique file name for writing the crash dump data to a file.
    // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
    // driver release) we may see redundant crash dumps. As a workaround,
    // attach a unique count to each generated file name.
    static int count = 0;
    const AZStd::string baseFileName =
        AZStd::string(applicationName.data())
        + "-"
        + AZStd::string::format("%i", baseInfo.pid)
        +"-"
        + AZStd::string::format("%i", ++count);


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
        ShaderInstructionsLookupCallback,
        ShaderSourceDebugInfoLookupCallback,
        this,
        &jsonSize);
    AssertOnError(result);

    // Step 2: Allocate a buffer and fetch the generated JSON.
    AZStd::vector<char> json(jsonSize);
    result = GFSDK_Aftermath_GpuCrashDump_GetJSON(
        decoder,
        uint32_t(json.size()),
        json.data());
    AssertOnError(result);

    // Write the the crash dump data as JSON to a file.
    const AZStd::string jsonFileName = crashDumpFileName + ".json";
    AZ::IO::SystemFile jsonFile;
    jsonFile.Open(jsonFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
    jsonFile.Write(json.data(), json.size());
    jsonFile.Close();

    // Destroy the GPU crash dump decoder object.
    result = GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder);
    AssertOnError(result);
}

void GpuCrashTracker::WriteShaderDebugInformationToFile(
    [[maybe_unused]] GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
    [[maybe_unused]] const void* pShaderDebugInfo,
    [[maybe_unused]] const uint32_t shaderDebugInfoSize)
{
    if (!m_initialized)
    {
        return;
    }

    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::OnShaderDebugInfoLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::OnShaderLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderHash& shaderHash,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderBinary) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
}

void GpuCrashTracker::OnShaderInstructionsLookup(
    [[maybe_unused]] const GFSDK_Aftermath_ShaderInstructionsHash& shaderInstructionsHash,
    [[maybe_unused]] PFN_GFSDK_Aftermath_SetData setShaderBinary) const
{
    // [GFX TODO][ATOM-14662] Add support for debug shader symbols
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
    const GFSDK_Aftermath_ShaderHash* pShaderHash,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderLookup(*pShaderHash, setShaderBinary);
}

void GpuCrashTracker::ShaderInstructionsLookupCallback(
    const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
    PFN_GFSDK_Aftermath_SetData setShaderBinary,
    void* pUserData)
{
    GpuCrashTracker* pGpuCrashTracker = reinterpret_cast<GpuCrashTracker*>(pUserData);
    pGpuCrashTracker->OnShaderInstructionsLookup(*pShaderInstructionsHash, setShaderBinary);
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

