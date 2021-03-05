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
#include <CameraFramework/ICameraLookAtBehavior.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Memory/SystemAllocator.h>

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
        AZ_CLASS_ALLOCATOR(SlideAlongAxisBasedOnAngle, AZ::SystemAllocator, 0); ///< Use AZ::SystemAllocator, otherwise a CryEngine allocator will be used. This will cause the Asset Processor to crash when this object is deleted, because of the wrong uninitialisation order
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraLookAtBehavior
        void AdjustLookAtTarget(float deltaTime, const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        RelativeAxisType m_axisToSlideAlong = ForwardBackward;
        EulerAngleType m_angleTypeToChangeFor = Pitch;
        VectorComponentType m_vectorComponentToIgnore = None;
        float m_maximumPositiveSlideDistance = 0.0f;
        float m_maximumNegativeSlideDistance = 0.0f;
    };
} // namespace Camera