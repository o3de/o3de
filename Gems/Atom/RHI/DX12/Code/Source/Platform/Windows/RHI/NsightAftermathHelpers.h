/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include <AzCore/Debug/Trace.h>


inline void AssertOnError(GFSDK_Aftermath_Result result)
{
    if(!GFSDK_Aftermath_SUCCEED(result))                                                              
    {
        AZStd::string errorMessage;
        switch (result)
        {
        case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
        {
            errorMessage = "Unsupported driver version - requires at least an NVIDIA R435 display driver.";
            break;
        }
        case GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported:
        {
            errorMessage = "Aftermath is incompatible with D3D API interception, such as PIX or Nsight Graphics.";
            break;
        }
        default:
            errorMessage = AZStd::string::format("Aftermath Error 0x%x", result);
        }
        AZ_Assert(false, "Aftermath Error: %s", errorMessage.c_str());
    }
}

