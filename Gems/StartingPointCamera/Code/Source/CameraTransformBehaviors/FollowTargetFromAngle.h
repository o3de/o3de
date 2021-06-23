/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
