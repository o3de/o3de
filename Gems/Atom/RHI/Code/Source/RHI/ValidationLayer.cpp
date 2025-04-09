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

namespace AZ::RHI
{
    AZ_CVAR(
        bool,
        r_debugBuildDeviceValidationOverride,
        true,
        nullptr,
        ConsoleFunctorFlags::Null,
        "Use this cvar to override device validation for debug builds.");

    ValidationMode ReadValidationMode()
    {
#if defined(AZ_RELEASE_BUILD)
        // Always disabled in Release configuration.
        return ValidationMode::Disabled;
#else
        const char* ValidationCommandLineOption = "rhi-device-validation";
        const char* ValidationSetting = "/O3DE/Atom/rhi-device-validation";

        ValidationMode mode = ValidationMode::Disabled;

        // Always enabled in Debug configuration by default, unless overriden by cvar, command line or setting registry.
        if constexpr (AZ::RHI::BuildOptions::IsDebugBuild)
        {
            mode = r_debugBuildDeviceValidationOverride ? ValidationMode::Enabled : ValidationMode::Disabled;
        }

        AZStd::string validationValue;

        // Check command line option first.
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);
        if (commandLine)
        {
            validationValue = RHI::GetCommandLineValue(ValidationCommandLineOption);
        }

        // Check settings registry next.
        if (validationValue.empty())
        {
            if (AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get())
            {
                settingsRegistry->Get(validationValue, ValidationSetting);
            }
        }

        if (!validationValue.empty())
        {
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
        }

        return mode;
#endif // defined(AZ_RELEASE_BUILD)
    }
}

