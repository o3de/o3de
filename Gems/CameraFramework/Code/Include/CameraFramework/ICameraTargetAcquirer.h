/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
