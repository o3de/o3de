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
    /// This class is responsible for giving the camera a place to start.  An example would be LocalPlayerAcquirer
    //////////////////////////////////////////////////////////////////////////
    class ICameraTargetAcquirer
        : public ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraTargetAcquirer, "{350C8DA8-732B-42F6-8EBD-70F8D5497436}", ICameraSubComponent);
        virtual ~ICameraTargetAcquirer() = default;
        /// Assign the transform of the desired target to outTransformInformation
        virtual bool AcquireTarget(AZ::Transform& outTransformInformation) = 0;
    };
} //namespace Camera