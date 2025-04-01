/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <CameraFramework/ICameraLookAtBehavior.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <LmbrCentral/Scripting/GameplayNotificationBus.h>
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Memory/SystemAllocator.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This will rotate the camera LookAt transform.  If you have a camera that
    /// is closely following a target, say in third person perspective you
    /// would not want the target to pitch while looking up and down.  You may
    /// also desire the ability to swivel the camera around the target while
    /// the target remains stationary.
    //////////////////////////////////////////////////////////////////////////
    class RotateCameraLookAt
        : public ICameraLookAtBehavior
        , public AZ::GameplayNotificationBus::Handler
    {
    public:
        ~RotateCameraLookAt() override = default;
        AZ_RTTI(RotateCameraLookAt, "{B72C5BE7-2DAF-412B-BBBB-F216B3DFB9A0}", ICameraLookAtBehavior);
        AZ_CLASS_ALLOCATOR(RotateCameraLookAt, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraLookAtBehavior
        void AdjustLookAtTarget(float deltaTime, const AZ::Transform& targetTransform, AZ::Transform& outLookAtTargetTransform) override;
        void Activate(AZ::EntityId) override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::GameplayNotificationBus
        void OnEventBegin(const AZStd::any&) override;
        void OnEventUpdating(const AZStd::any&) override;
        void OnEventEnd(const AZStd::any&) override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        AxisOfRotation m_axisOfRotation = AxisOfRotation::X_Axis;
        AZStd::string m_eventName = "";
        float m_rotationSpeedScale = 1.f;
        bool m_shouldInvertAxis = false;

        //////////////////////////////////////////////////////////////////////////
        // internal data
        float m_rotationAmount = 0.f;
        AZ::EntityId m_rigEntity;
    };
} // namespace Camera
