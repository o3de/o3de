/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <CameraFramework/ICameraLookAtBehavior.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This will slide the look at target along a desired axis based on a
    /// particular Euler angle.  As an example setting this up with ForwardBackward
    /// and Pitch, the more the target pitches the further forward it will slide.
    /// This will have the behavior that when looking down you will be looking
    /// down ahead of the target instead of directly at the top.  A similar result
    /// will occur when looking up.  This could also be used for peeking around
    /// corners.  This is primarily useful for third person cameras.
    //////////////////////////////////////////////////////////////////////////
    class SlideAlongAxisBasedOnAngle
        : public ICameraLookAtBehavior
    {
    public:
        ~SlideAlongAxisBasedOnAngle() override = default;
        AZ_RTTI(SlideAlongAxisBasedOnAngle, "{8DDA8D0B-5BC3-437E-894B-5144E6E81236}", ICameraLookAtBehavior);
        AZ_CLASS_ALLOCATOR(SlideAlongAxisBasedOnAngle, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraLookAtBehavior
        void AdjustLookAtTarget(float deltaTime, const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

        bool XAndYIgnored() const;
        bool XAndZIgnored() const;
        bool YAndZIgnored() const;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        RelativeAxisType m_axisToSlideAlong = ForwardBackward;
        EulerAngleType m_angleTypeToChangeFor = Pitch;
        float m_maximumPositiveSlideDistance = 0.0f;
        float m_maximumNegativeSlideDistance = 0.0f;
        bool m_ignoreX = false;
        bool m_ignoreY = false;
        bool m_ignoreZ = false;
    };
} // namespace Camera
