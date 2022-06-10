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
    void SetAftermathEventMarker( [[maybe_unused]] void* cntxHandle, [[maybe_unused]] const char* markerData, [[maybe_unused]] bool isAftermathInitialized)
    {
#if defined(USE_NSIGHT_AFTERMATH)
        if (isAftermathInitialized)
        {
            GFSDK_Aftermath_Result result = GFSDK_Aftermath_SetEventMarker(
                static_cast<GFSDK_Aftermath_ContextHandle>(cntxHandle), static_cast<const void*>(markerData),
                static_cast<unsigned int>(strlen(markerData) + 1));
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

