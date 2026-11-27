/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRUtils.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Settings/SettingsRegistry.h>

static constexpr char OpenXREnableSetting[] = "/O3DE/Atom/OpenXR/Enable";

namespace XR
{
    AZ::Matrix4x4 CreateStereoscopicProjection(float angleLeft, float angleRight, float angleBottom, float angleTop, float nearDist, float farDist, bool reverseDepth)
    {
        AZ::Matrix4x4 result;
        AZ_MATH_ASSERT(nearDist > 0.0f, "Near plane distance must be greater than 0");
        
        const float left = tanf(angleLeft);
        const float right = tanf(angleRight);
        const float bottom = tanf(angleBottom);
        const float top = tanf(angleTop);

        const float invfn = 1.0f / (farDist - nearDist);
        const float tanAngleWidth = right - left;
        const float tanAngleHeight = (top - bottom);

        //Stretch more horizontally and vertically
        //Generate asymmetric or off-center projection matrix
        //Right handed coord system as Openxr provides data in that system
        //Handle the case where farDist is less than nearDist
        if(farDist > nearDist)
        { 
            result.SetRow(0, 2.0f / tanAngleWidth, 0.0f, (left + right) / tanAngleWidth, 0.0f);
            result.SetRow(1, 0.0f, 2.0f / tanAngleHeight, (top + bottom) / tanAngleHeight, 0.0f);
            if(reverseDepth)
            { 
                result.SetRow(2, 0.0f, 0.0f, 2.0f * nearDist * invfn, 2.0f * farDist * nearDist * invfn);
            }
            else
            {
                result.SetRow(2, 0.0f, 0.0f, -1.0f * (farDist + nearDist) * invfn, -2.0f * farDist * nearDist * invfn);
            }
            result.SetRow(3, 0.0f, 0.0f, -1.0f, 0.0f);
        }
        else
        {
            // place the far plane at infinity
            result.SetRow(0, 2.0f / tanAngleWidth, 0.0f, (left + right) / tanAngleWidth, 0.0f);
            result.SetRow(1, 0.0f, 2.0f / tanAngleHeight, (top + bottom) / tanAngleHeight, 0.0f);
            if (reverseDepth)
            {
                result.SetRow(2, 0.0f, 0.0f, 0.0f, 2.0f * nearDist);
            }
            else
            {
                result.SetRow(2, 0.0f, 0.0f, -1.0f, -2.0f * nearDist);
            }
            result.SetRow(3, 0.0f, 0.0f, -1.0f, 0.0f);
        }

        return result;
    }

    bool IsOpenXREnabled()
    {
        const AzFramework::CommandLine* commandLine = nullptr;
        AZStd::string commandLineValue;

        //Check command line option(-openxr=enable)
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);
        if (commandLine)
        {
            if (size_t switchCount = commandLine->GetNumSwitchValues("openxr"); switchCount > 0)
            {
                commandLineValue = commandLine->GetSwitchValue("openxr", switchCount - 1);
            }
        }
        bool isOpenXREnabledViaCL = AzFramework::StringFunc::Equal(commandLineValue.c_str(), "enable");

        //Check settings registry
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        bool isOpenXREnabledViaSR = false;
        if (settingsRegistry)
        {
            settingsRegistry->Get(isOpenXREnabledViaSR, OpenXREnableSetting);
        }
        return isOpenXREnabledViaSR || isOpenXREnabledViaCL;
    }
}
