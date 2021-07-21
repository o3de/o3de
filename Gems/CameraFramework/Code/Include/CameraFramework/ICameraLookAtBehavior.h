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
    /// This class is responsible for mutating the LookAt position based on the target's new transform
    //////////////////////////////////////////////////////////////////////////
    class ICameraLookAtBehavior
        : public ICameraSubComponent
    {
    public:
        AZ_RTTI(ICameraLookAtBehavior, "{B15B9078-4090-4661-84DB-D37E0DFA1D3A}", ICameraSubComponent);
        virtual ~ICameraLookAtBehavior() = default;
        /// Adjust the outLookAtTargetTransform based on the target's initial transform and the time that's passed since the last call
        virtual void AdjustLookAtTarget(float deltaTime, const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform) = 0;
    };
} //namespace LYGame
