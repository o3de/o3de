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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySystem_precompiled.h"
#include "HMDCVars.h"
#include <HMDBus.h>
#include <sstream>

namespace AZ
{
namespace VR
{

void HMDCVars::OnHMDRecenter([[maybe_unused]] IConsoleCmdArgs* args)
{
    EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, RecenterPose);
}

void HMDCVars::OnHMDTrackingLevelChange(IConsoleCmdArgs* args)
{
    if (args->GetArgCount() != 2)
    {
        // First arg should be the command itself, second arg should be the requested tracking level.
        return;
    }

    // Read the new tracking level.
    int argVal = 0;
    std::stringstream stream(args->GetArg(1));
    stream >> argVal;

    AZ::VR::HMDTrackingLevel level = static_cast<AZ::VR::HMDTrackingLevel>(argVal);
    EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, SetTrackingLevel, level);
}

void HMDCVars::OnOutputToHMDChanged(ICVar* var)
{
    // Set the necessary cvars for turning on/off output to an HMD.
    ICVar* mode   = gEnv->pConsole->GetCVar("r_StereoMode");
    ICVar* output = gEnv->pConsole->GetCVar("r_StereoOutput");
    ICVar* height = gEnv->pConsole->GetCVar("r_height");
    ICVar* width  = gEnv->pConsole->GetCVar("r_width");

    if (!mode || !output || !height || !width)
    {
        return;
    }

    bool enable = (var->GetIVal() == 1);
    if (enable)
    {
        // Auto-set the resolution.
        {
            const AZ::VR::HMDDeviceInfo* deviceInfo = nullptr;
            EBUS_EVENT_RESULT(deviceInfo, AZ::VR::HMDDeviceRequestBus, GetDeviceInfo);

            // If the device info exists then there is a VR device connected and working.
            if (deviceInfo)
            {
                mode->Set(EStereoMode::STEREO_MODE_DUAL_RENDERING);
                output->Set(EStereoOutput::STEREO_OUTPUT_HMD);

                width->Set(static_cast<int>(deviceInfo->renderWidth));
                height->Set(static_cast<int>(deviceInfo->renderHeight));
            }
        }
    }
    else
    {
        mode->Set(EStereoMode::STEREO_MODE_NO_STEREO);
        output->Set(EStereoOutput::STEREO_OUTPUT_STANDARD);
    }
}

void HMDCVars::OnHMDDebugInfo(ICVar* var)
{
    bool enable = (var->GetIVal() == 1);
    EBUS_EVENT(AZ::VR::HMDDebuggerRequestBus, EnableInfo, enable);
}

void HMDCVars::OnHMDDebugCamera(ICVar* var)
{
    bool enable = (var->GetIVal() == 1);
    EBUS_EVENT(AZ::VR::HMDDebuggerRequestBus, EnableCamera, enable);
}

int HMDCVars::hmd_social_screen = static_cast<int>(HMDSocialScreen::UndistortedLeftEye);
int HMDCVars::hmd_debug_info = 0;
int HMDCVars::hmd_debug_camera = 0;

} // namespace VR
} // namespace AZ
