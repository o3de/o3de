/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>


namespace Aftermath
{
    bool InitializeAftermath([[maybe_unused]] AZ::RHI::Ptr<ID3D12DeviceX> dx12Device)
    {
#if defined(USE_NSIGHT_AFTERMATH)
        // Initialize Nsight Aftermath for this device.
        //
        // * EnableMarkers - this will include information about the Aftermath
        //   event marker nearest to the crash.
        //
        //   Using event markers should be considered carefully as they can cause
        //   considerable CPU overhead when used in high frequency code paths.
        //
        // * EnableResourceTracking - this will include additional information about the
        //   resource related to a GPU virtual address seen in case of a crash due to a GPU
        //   page fault. This includes, for example, information about the size of the
        //   resource, its format, and an indication if the resource has been deleted.
        //
        // * CallStackCapturing - this will include call stack and module information for
        //   the draw call, compute dispatch, or resource copy nearest to the crash.
        //
        //   Using this option should be considered carefully. Enabling call stack
        //   capturing will cause very high CPU overhead.
        //
        // * GenerateShaderDebugInfo - this instructs the shader compiler to
        //   generate debug information (line tables) for all shaders. Using this option
        //   should be considered carefully. It may cause considerable shader compilation
        //   overhead and additional overhead for handling the corresponding shader debug
        //   information callbacks.
        //
        const uint32_t aftermathFlags =
            GFSDK_Aftermath_FeatureFlags_EnableMarkers |             // Enable event marker tracking.
            GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |    // Enable tracking of resources.
            GFSDK_Aftermath_FeatureFlags_CallStackCapturing |         // Capture call stacks for all draw calls, compute dispatches, and resource copies.
            GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;    // Generate debug information for shaders.

        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, aftermathFlags, dx12Device.get());
        AssertOnError(result);
        return GFSDK_Aftermath_SUCCEED(result);
#else
        return false;
#endif
    }

    void SetAftermathEventMarker( [[maybe_unused]] void* cntxHandle, [[maybe_unused]] const AZStd::string& markerData, [[maybe_unused]] bool isAftermathInitialized)
    {
#if defined(USE_NSIGHT_AFTERMATH)
        if (isAftermathInitialized)
        {
            GFSDK_Aftermath_Result result = GFSDK_Aftermath_SetEventMarker(
                static_cast<GFSDK_Aftermath_ContextHandle>(cntxHandle), static_cast<const void*>(markerData.c_str()),
                static_cast<unsigned int>(markerData.size()) + 1);
            AssertOnError(result);
        }
#endif
    }

    void* CreateAftermathContextHandle([[maybe_unused]] ID3D12GraphicsCommandList* commandList, [[maybe_unused]] void* crashTracker)
    {
#if defined(USE_NSIGHT_AFTERMATH)
        GFSDK_Aftermath_ContextHandle aftermathCntHndl = nullptr;
        // Create an Nsight Aftermath context handle for setting Aftermath event markers in this command list.
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_CreateContextHandle(commandList, &aftermathCntHndl);
        AssertOnError(result);
        static_cast<GpuCrashTracker*>(crashTracker)->AddContext(aftermathCntHndl);
        return static_cast<void*>(aftermathCntHndl);
#else
        return nullptr;
#endif
    }

    void OutputLastScopeExecutingOnGPU([[maybe_unused]] void* crashTracker)
    {
#if defined(USE_NSIGHT_AFTERMATH)
        AZStd::vector<GFSDK_Aftermath_ContextHandle> cntxtHandles = static_cast<GpuCrashTracker*>(crashTracker)->GetContextHandles();
        GFSDK_Aftermath_ContextData* outContextData = new GFSDK_Aftermath_ContextData[cntxtHandles.size()];
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_GetData(static_cast<uint32_t>(cntxtHandles.size()), cntxtHandles.data(), outContextData);
        AssertOnError(result);
        for (int i = 0; i < cntxtHandles.size(); i++)
        {
            if (outContextData[i].status == GFSDK_Aftermath_Context_Status_Executing)
            {
                AZ_Warning("RHI::DX12", false, "\n***************GPU was executing \"%s\" pass when it crashed***********************\n", static_cast<const char*>(outContextData[i].markerData));
            }
        }
#endif
    }
}

