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
#pragma once
#include "CameraFramework/ICameraSubComponent.h"

namespace AZ
{
    class Transform;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This class represents the behavior that will change the cameras transform based on the target.
    /// An example would be a behavior that follows the target from above
    //////////////////////////////////////////////////////////////////////////
    class ICameraTransformBehavior
        : public ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraTransformBehavior, "{6FCA64C8-6CA5-4CEA-B10F-BB5ED3FB6561}", ICameraSubComponent);
        virtual ~ICameraTransformBehavior() = default;
        /// Adjust the camera's final transform in outCameraTransform using the target's transform, the camera's initial transform and the time that's passed since the last call
        virtual void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& outCameraTransform) = 0;
    };
} //namespace Camera