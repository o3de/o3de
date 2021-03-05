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
#include <CameraFramework/ICameraTransformBehavior.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Memory/SystemAllocator.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This Camera Transform Behavior will follow the target transform from
    /// a given angle of Yaw, Pitch or Roll
    //////////////////////////////////////////////////////////////////////////
    class FollowTargetFromAngle
        : public ICameraTransformBehavior
    {
    public:
        ~FollowTargetFromAngle() override = default;
        AZ_RTTI(FollowTargetFromAngle, "{4DBE7A2C-8E93-422E-8942-9601A270D37E}", ICameraTransformBehavior)
        AZ_CLASS_ALLOCATOR(FollowTargetFromAngle, AZ::SystemAllocator, 0); ///< Use AZ::SystemAllocator, otherwise a CryEngine allocator will be used. This will cause the Asset Processor to crash when this object is deleted, because of the wrong uninitialisation order
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTransformBehavior
        void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform) override;
        void Activate(AZ::EntityId) override {}
        void Deactivate() override {}

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        float m_angleInDegrees = 0.f;
        EulerAngleType m_rotationType = EulerAngleType::Pitch;
        float m_distanceFromTarget = 1.0f;
    };
} //namespace Camera