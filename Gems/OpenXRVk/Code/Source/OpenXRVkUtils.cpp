/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkUtils.h>
#include <AzCore/Debug/Trace.h>

namespace OpenXRVk
{
    AZ::RHI::ResultCode ConvertResult(XrResult xrResult)
    {
        switch (xrResult)
        {
        case XR_SUCCESS:
            return AZ::RHI::ResultCode::Success;
        case XR_ERROR_OUT_OF_MEMORY:
            return AZ::RHI::ResultCode::OutOfMemory;
        default:
            return AZ::RHI::ResultCode::Fail;
        }
    }

    bool IsSuccess(XrResult result)
    {
        if (result != XR_SUCCESS)
        {
            AZ_Error("XR", false, "ERROR: XR API method failed: %s", GetResultString(result));
            return false;
        }
        return true;
    }

    bool IsError(XrResult result)
    {
        return IsSuccess(result) == false;
    }

    const char* GetResultString(const XrResult result)
    {
        return to_string(result);
    }

    void PrintXrError([[maybe_unused]]const char* windowName, [[maybe_unused]]const XrResult error, const char* fmt, ...)
    {
        va_list argList;
        va_start(argList, fmt);

        const auto subMsg = AZStd::string::format_arg(fmt, argList);
        AZ_Error(windowName, false, "%s. Got OpenXR Error: %s\n", subMsg.c_str(), GetResultString(error));

        va_end(argList);
    }

    XR::RawStringList FilterList(const XR::RawStringList& source, const XR::StringList& filter)
    {
        XR::RawStringList filteredList;
        for (auto& item : source)
        {
            if (AZStd::find(filter.begin(), filter.end(), item) != filter.end())
            {
                filteredList.push_back(item);
            }
        }
        return filteredList;
    }

    AZStd::vector<const char*> ParseExtensionString(char* names)
    {
        AZStd::vector<const char*> list;
        while (*names != 0)
        {
            list.push_back(names);
            while (*(++names) != 0)
            {
                if (*names == ' ')
                {
                    *names++ = '\0';
                    break;
                }
            }
        }
        return list;
    }

    AZ::Quaternion AzQuaternionFromXrPose(const XrPosef& pose, bool convertCoordinates)
    {
        if (convertCoordinates)
        {
            return AZ::Quaternion(pose.orientation.x, -pose.orientation.z, pose.orientation.y, pose.orientation.w);
        }
        return AZ::Quaternion(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
    }

    AZ::Vector3 AzVector3FromXrVector3(const XrVector3f& xrVec3, bool convertCoordinates)
    {
        return AZ::Vector3(xrVec3.x,
                           convertCoordinates ? -xrVec3.z : xrVec3.y,
                           convertCoordinates ?  xrVec3.y : xrVec3.z);
    }

    AZ::Vector3 AzPositionFromXrPose(const XrPosef& pose, bool convertCoordinates)
    {
        return AzVector3FromXrVector3(pose.position, convertCoordinates);
    }

    AZ::Transform AzTransformFromXrPose(const XrPosef& pose, bool convertCoordinates)
    {
        const auto azQuat = AzQuaternionFromXrPose(pose, convertCoordinates);
        const auto azVec = AzPositionFromXrPose(pose, convertCoordinates);
        AZ::Transform tm = AZ::Transform::CreateFromQuaternionAndTranslation(azQuat, azVec);
        return tm;
    }

    XrPosef XrPoseFromAzTransform(const AZ::Transform& tm, bool convertCoordinates)
    {
        const auto &azQuat = tm.GetRotation();
        const auto &azPos = tm.GetTranslation();
        
        XrPosef pose;
        
        pose.orientation.x = azQuat.GetX();
        pose.orientation.y = convertCoordinates ? -azQuat.GetZ() : azQuat.GetY();
        pose.orientation.z = convertCoordinates ? azQuat.GetY() : azQuat.GetZ();
        pose.orientation.w = azQuat.GetW();

        pose.position.x = azPos.GetX();
        pose.position.y = convertCoordinates ? -azPos.GetZ() : azPos.GetY();
        pose.position.z = convertCoordinates ?  azPos.GetY() : azPos.GetZ();

        return pose; 
    }

    AZStd::string ConvertXrPathToString(XrInstance xrInstance, XrPath xrPath)
    {
        if ((xrInstance == XR_NULL_HANDLE) || (xrPath == XR_NULL_PATH))
        {
            return AZStd::string("");
        }
        constexpr uint32_t MaxBytes = 256;
        char pathAsChars[MaxBytes];
        uint32_t usedBytes = 0;
        XrResult result = xrPathToString(xrInstance, xrPath, MaxBytes, &usedBytes, pathAsChars);
        if (IsError(result))
        {
            PrintXrError("OpenXRVkUtils", result, "Failed to convert xrPath to a string with %u bytes.", MaxBytes);
            return AZStd::string("");
        }
        return AZStd::string(pathAsChars);
    }


    float ReadActionHandleFloat(IOpenXRActions* iface, IOpenXRActions::ActionHandle actionHandle, float deadZone)
    {
        auto outcome = iface->GetActionStateFloat(actionHandle);
        if (!outcome.IsSuccess())
        {
            // Most likely the controller went to sleep.
            return 0.0f;
        }
        float value = outcome.GetValue();
        if (fabsf(value) < deadZone)
        {
            return 0.0f;
        }
        return value;
    }
}
