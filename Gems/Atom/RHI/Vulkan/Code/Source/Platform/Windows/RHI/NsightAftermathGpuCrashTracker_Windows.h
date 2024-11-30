/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#if defined(USE_NSIGHT_AFTERMATH)
#include <RHI/NsightAftermathHelpers.h>

//! Implements GPU crash dump tracking using the Nsight
//! Aftermath API. It lets you add markers which can be used to identify
//! the scope that was executing on the GPU at the time of TDR/crash
class GpuCrashTracker
{
public:
    GpuCrashTracker() = default;
    ~GpuCrashTracker();

    //! Initialize the GPU crash dump tracker.
    void EnableGPUCrashDumps();

private:

    //! *********************************************************
    //! Callback handlers for GPU crash dumps and related data.
    //!

    //! Handler for GPU crash dump callbacks.
    void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

    //! Handler for shader debug information callbacks.
    void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);

    //! Handler for GPU crash dump description callbacks.
    void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription);

    //! *********************************************************
    //! Helpers for writing a GPU crash dump and debug information
    //! data to files.
    //!

    //! Helper for writing a GPU crash dump to a file.
    void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);

    //! Helper for writing shader debug information to a file
    void WriteShaderDebugInformationToFile(
        GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
        const void* pShaderDebugInfo,
        const uint32_t shaderDebugInfoSize);

    //! *********************************************************
    //! Helpers for decoding GPU crash dump to JSON.
    //!

    //! Handler for shader debug information lookup callbacks.
    //! This is used by the JSON decoder for mapping shader instruction
    //! addresses to SPIR-V lines or HLSl source lines.
    void OnShaderDebugInfoLookup(
        const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
        PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const;

    //! Handler for shader lookup callbacks.
    //! This is used by the JSON decoder for mapping shader instruction
    //! addresses to SPIR-V lines or HLSL source lines.
    //! NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
    //! Aftermath will require access to both the stripped and the not stripped
    //! shader binaries.
    void OnShaderLookup(
        const GFSDK_Aftermath_ShaderBinaryHash& shaderHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary) const;


    //! Marker resolve callback
    void OnResolveMarker(
        const void* pMarker,
        const uint32_t markerDataSize,
        void** resolvedMarkerData,
        uint32_t* markerSize) const;


    //! Handler for shader source debug info lookup callbacks.
    //! This is used by the JSON decoder for mapping shader instruction addresses to
    //! HLSL source lines, if the shaders used by the application were compiled with
    //! separate debug info data files.
    void OnShaderSourceDebugInfoLookup(
        const GFSDK_Aftermath_ShaderDebugName& shaderDebugName,
        PFN_GFSDK_Aftermath_SetData setShaderBinary) const;

    //! *********************************************************
    //! Static callback wrappers.
    //!

    //! GPU crash dump callback.
    static void GpuCrashDumpCallback(
        const void* pGpuCrashDump,
        const uint32_t gpuCrashDumpSize,
        void* pUserData);

    //! Shader debug information callback.
    static void ShaderDebugInfoCallback(
        const void* pShaderDebugInfo,
        const uint32_t shaderDebugInfoSize,
        void* pUserData);

    //! GPU crash dump description callback.
    static void CrashDumpDescriptionCallback(
        PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
        void* pUserData);

    //! Shader debug information lookup callback.
    static void ShaderDebugInfoLookupCallback(
        const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
        PFN_GFSDK_Aftermath_SetData setShaderDebugInfo,
        void* pUserData);

    //! Shader lookup callback.
    static void ShaderLookupCallback(
        const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash,
        PFN_GFSDK_Aftermath_SetData setShaderBinary,
        void* pUserData);

    //! Marker resolve callback
    static void ResolveMarkerCallback(
        const void* pMarker,
        const uint32_t markerDataSize,
        void* pUserData,
        void** resolvedMarkerData,
        uint32_t* markerSize);

    //! Shader source debug info lookup callback.
    static void ShaderSourceDebugInfoLookupCallback(
        const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
        PFN_GFSDK_Aftermath_SetData setShaderBinary,
        void* pUserData);

    //! *********************************************************
    //! GPU crash tracker state.
    //!

    //! Is the GPU crash dump tracker initialized?
    bool m_initialized = false;

    //! For thread-safe access of GPU crash tracker state.
    mutable AZStd::mutex m_mutex;

    // cache executable folder and project name
    AZStd::string m_executableFolder;
    AZStd::string m_projectName;
};

#endif
