/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/RHIUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzCore/Console/Console.h>

namespace AZ
{
    namespace RHI
    {
        AZ_CVAR(
            bool,
            r_debugBuildDeviceValidationOverride,
            true,
            nullptr,
            ConsoleFunctorFlags::Null,
            "Use this cvar to override device validation for debug builds.");

        static const char* ValidationCommandLineOption = "rhi-device-validation";

        AZStd::optional<ValidationMode> ReadValidationModeFromCommandArgs()
        {
            ValidationMode mode = ValidationMode::Disabled;
            bool debugBuildDeviceValidationOverride = true;
            if (auto* console = Interface<IConsole>::Get())
            {
                console->GetCvarValue("r_debugBuildDeviceValidationOverride", debugBuildDeviceValidationOverride);
            }

            bool isDebugBuild = AZ::RHI::BuildOptions::IsDebugBuild;
            if (isDebugBuild && debugBuildDeviceValidationOverride)
            {
                mode = ValidationMode::Enabled;
            }

            const AzFramework::CommandLine* commandLine = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);
            if (commandLine)
            {
                AZStd::string validationValue = RHI::GetCommandLineValue(ValidationCommandLineOption);
                
                if (AzFramework::StringFunc::Equal(validationValue.c_str(), "disable"))
                {
                    mode = ValidationMode::Disabled;
                }
                else if (AzFramework::StringFunc::Equal(validationValue.c_str(), "enable"))
                {
                    mode = ValidationMode::Enabled;
                }
                else if (AzFramework::StringFunc::Equal(validationValue.c_str(), "verbose"))
                {
                    mode = ValidationMode::Verbose;
                }
                else if (AzFramework::StringFunc::Equal(validationValue.c_str(), "gpu"))
                {
                    mode = ValidationMode::GPU;
                }
                return AZStd::optional<ValidationMode>(mode);
            }
            else
            {
                return AZStd::optional<ValidationMode>();
            }
        }
    }
}

