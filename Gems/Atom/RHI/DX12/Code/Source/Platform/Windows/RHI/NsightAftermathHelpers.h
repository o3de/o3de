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
* This software contains source code provided by NVIDIA Corporation.
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

