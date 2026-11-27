/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <OpenXRVk_Platform.h>
#include <XR/XRBase.h>
#include "OpenXRVkActionsInterface.h"


// Macro to generate stringify functions for OpenXR enumerations based data provided in openxr_reflection.h
#define ENUM_CASE_STR(name, val) case name: return #name;
#define MAKE_XR_TO_STRING_FUNC(enumType)                  \
    inline const char* to_string(enumType e) {         \
        switch (e) {                                   \
            XR_LIST_ENUM_##enumType(ENUM_CASE_STR)     \
            default: return "Unknown " #enumType;      \
        }                                              \
    }


MAKE_XR_TO_STRING_FUNC(XrReferenceSpaceType);
MAKE_XR_TO_STRING_FUNC(XrViewConfigurationType);
MAKE_XR_TO_STRING_FUNC(XrEnvironmentBlendMode);
MAKE_XR_TO_STRING_FUNC(XrSessionState);
MAKE_XR_TO_STRING_FUNC(XrResult);
MAKE_XR_TO_STRING_FUNC(XrFormFactor);

namespace OpenXRVk
{

#define RETURN_XR_RESULT_IF_UNSUCCESSFUL(result) \
    if ((result) != XR_SUCCESS) {\
        return (result);\
    }

#define RETURN_IF_UNSUCCESSFUL(result) \
    if ((result) != XR_SUCCESS) {\
        return;\
    }

#define WARN_IF_UNSUCCESSFUL(result) \
    if ((result) != XR_SUCCESS) {\
        AZ_Warning("OpenXRVk", false, "Warning error code: %s", to_string(result));\
    }

#define ASSERT_IF_UNSUCCESSFUL(result) \
    if ((result) != XR_SUCCESS) {\
        AZ_Assert(false, "Assert error code: %s", to_string(result));\
    }


    AZ::RHI::ResultCode ConvertResult(XrResult xrResult);
    bool IsSuccess(XrResult result);
    bool IsError(XrResult result);
    const char* GetResultString(const XrResult result);
    void PrintXrError(const char* windowName, const XrResult error, const char* fmt, ...);
    XR::RawStringList FilterList(const XR::RawStringList& source, const XR::StringList& filter);
    AZStd::string ConvertXrPathToString(XrInstance xrInstance, XrPath xrPath);

    //! Input is an array of chars with multiple ' ' char embedded in it, indicating the start of a new string.
    //! Iterate through the characters while caching the starting pointer to a string
    //! and every time ' ' is encountered replace it with '\0' to indicate the end of a string.
    AZStd::vector<const char*> ParseExtensionString(char* names);

    AZ::Quaternion AzQuaternionFromXrPose(const XrPosef& pose, bool convertCoordinates = true);
    AZ::Vector3 AzVector3FromXrVector3(const XrVector3f& xrVec3, bool convertCoordinates = true);
    AZ::Vector3 AzPositionFromXrPose(const XrPosef& pose, bool convertCoordinates = true);
    AZ::Transform AzTransformFromXrPose(const XrPosef& pose, bool convertCoordinates = true);
    XrPosef XrPoseFromAzTransform(const AZ::Transform& tm, bool convertCoordinates = true);

    float ReadActionHandleFloat(IOpenXRActions* iface, IOpenXRActions::ActionHandle actionHandle, float deadZone = 0.05f);
}
