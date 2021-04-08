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
*/

#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/RHIUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace AZ
{
    namespace RHI
    {
        static const char* ValidationCommandLineOption = "rhi-device-validation";

        AZStd::optional<ValidationMode> ReadValidationModeFromCommandArgs()
        {
            ValidationMode mode = ValidationMode::Disabled;
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

