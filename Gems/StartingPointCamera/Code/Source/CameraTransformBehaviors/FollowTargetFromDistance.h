/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <CameraFramework/ICameraTransformBehavior.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Transform.h>
#include <LmbrCentral/Scripting/GameplayNotificationBus.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// This behavior will cause the camera to follow the target by "Follow Distance"
    /// meters.  Zoom using action events.  Use a distance of 0 for FPS style games
    /// and a distance greater than 0 for a Third Person style camera
    //////////////////////////////////////////////////////////////////////////
    class FollowTargetFromDistance
        : public ICameraTransformBehavior
        , public AZ::GameplayNotificationBus::MultiHandler
    {
    public:
        ~FollowTargetFromDistance() override = default;
        AZ_RTTI(FollowTargetFromDistance, "{E6BEDB2C-6812-4369-8C0F-C1E72F380E50}", ICameraTransformBehavior)
        AZ_CLASS_ALLOCATOR(FollowTargetFromDistance, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // ICameraTransformBehavior
        void AdjustCameraTransform(float deltaTime, const AZ::Transform& initialCameraTransform, const AZ::Transform& targetTransform, AZ::Transform& inOutCameraTransform) override;
        void Activate(AZ::EntityId) override;
        void Deactivate() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::GameplayNotificationBus
        void OnEventBegin(const AZStd::any&) override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // Editor helpers
        float GetMinimumFollowDistance() { return m_minFollowDistance; }
        float GetMaximumFollowDistance() { return m_maxFollowDistance; }

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        float m_minFollowDistance = 0.f;
        float m_followDistance = 0.f;
        float m_maxFollowDistance = 0.f;
        AZStd::string m_zoomInEventName = "";
        AZStd::string m_zoomOutEventName = "";
        AZ::EntityId m_channelId;
        float m_zoomSpeedScale = 1.f;
    };
}
