/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI.Profiler/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ::RHI
{
    bool ShouldLoadProfiler(const AZStd::string_view name)
    {
        const char* profilerOption = "rhi-gpu-profiler";
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

        if (commandLine)
        {
            return AzFramework::StringFunc::Equal(commandLine->GetSwitchValue(profilerOption), name);
        }
        return false;
    }
}
