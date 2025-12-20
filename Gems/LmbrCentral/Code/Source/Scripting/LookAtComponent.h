/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>

namespace LmbrCentral
{

    class LookAtComponentRequests
        : public AZ::ComponentBus
    {
    public:

        //! Get the target entity being looked at
        virtual AZ::EntityId GetTarget() const = 0;

        //! Set the target entity to look at
        virtual void SetTarget([[maybe_unused]] const AZ::EntityId& targetEntity) {}

        //! Get the target position being looked at
        virtual AZ::Vector3 GetTargetPosition() const = 0;

        //! Set the target position to look at
        virtual void SetTargetPosition([[maybe_unused]] const AZ::Vector3& position) {}

        //! Get the target Look At transform
        virtual AZ::Transform GetLookAtTransform() const = 0;

        //! Get the reference forward axis
        virtual AZ::Transform::Axis GetAxis() const = 0;

        //! Set the reference forward axis
        virtual void SetAxis([[maybe_unused]] AZ::Transform::Axis axis = AZ::Transform::Axis::ZPositive) {}

        //! Get the strength / quickness of the look at rotation
        virtual float GetStrength() const = 0;

        //! Set the strength / quickness of the look at rotation
        virtual void SetStrength([[maybe_unused]] const float strength) {};

        //! Get whether the pitch is fixated
        virtual bool GetFixatePitch() const = 0;

        //! Set whether the pitch is fixated
        virtual void SetFixatePitch([[maybe_unused]] const bool fixatePitch) {};

        //! Get whether the roll is fixated
        virtual bool GetFixateRoll() const = 0;

        //! Set whether the roll is fixated
        virtual void SetFixateRoll([[maybe_unused]] const bool fixateRoll) {};

        //! Get whether the yaw is fixated
        virtual bool GetFixateYaw() const = 0;

        //! Set whether the yaw is fixated
        virtual void SetFixateYaw([[maybe_unused]] const bool fixateYaw) {};

        //! Get whether the Look At component is enabled
        virtual bool GetEnabled() const = 0;

        //! Set whether the Look At component is enabled
        virtual void SetEnabled([[maybe_unused]] bool enabled) {}

        //! Get whether the Look At transform is applied
        virtual bool GetApplyLookAtTransform() const = 0;

        //! Set whether the Look At transform is applied
        virtual void SetApplyLookAtTransform([[maybe_unused]] bool applyLookAtTransform) {}
    };

    using LookAtComponentRequestBus = AZ::EBus<LookAtComponentRequests>;

    class LookAtComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        //! Notifies you that the target has changed
        virtual void OnTargetChanged(AZ::EntityId) { }
        virtual void OnEnabledChanged(bool) { }
    };

    using LookAtComponentNotificationBus = AZ::EBus<LookAtComponentNotifications>;

    //=========================================================================
    // LookAtComponent
    //=========================================================================
    class LookAtComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::MultiHandler
        , private AZ::TickBus::Handler
        , private AZ::EntityBus::Handler
        , private LookAtComponentRequestBus::Handler
    {
    public:
        friend class EditorLookAtComponent;

        AZ_COMPONENT(LookAtComponent, "{11CDC627-25A9-4760-A61F-576CDB189B38}");

        //=====================================================================
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        //=====================================================================

        //=====================================================================
        // TransformBus
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;
        //=====================================================================

        //=====================================================================
        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //=====================================================================

        //=====================================================================
        // EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;
        //=====================================================================

        //=====================================================================
        // LookAtComponentRequestBus
        AZ::EntityId GetTarget() const;
        void SetTarget(const AZ::EntityId& targetEntity) override;
        AZ::Vector3 GetTargetPosition() const override;
        void SetTargetPosition(const AZ::Vector3& targetPosition) override;
        AZ::Transform GetLookAtTransform() const override;
        AZ::Transform::Axis GetAxis() const override;
        void SetAxis(AZ::Transform::Axis axis) override;
        float GetStrength() const override;
        void SetStrength(const float strength) override;
        bool GetFixatePitch() const override;
        void SetFixatePitch(const bool fixatePitch) override;
        bool GetFixateRoll() const override;
        void SetFixateRoll(const bool fixateRoll) override;
        bool GetFixateYaw() const override;
        void SetFixateYaw(const bool fixateYaw) override;
        bool GetEnabled() const override;
        void SetEnabled(const bool enabled) override;
        bool GetApplyLookAtTransform() const override;
        void SetApplyLookAtTransform(const bool applyLookAtTransform) override;
        //=====================================================================

    protected:
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("LookAtService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("LookAtService"));
        }

    private:
        void RecalculateTransform(const float deltaTime);

        // Serialized data
        AZ::EntityId m_targetId = AZ::EntityId();
        AZ::Vector3 m_targetPosition = AZ::Vector3::CreateZero();
        AZ::Transform m_lookAtTransform = AZ::Transform::Identity();
        AZ::Transform::Axis m_forwardAxis = AZ::Constants::Axis::YPositive;
        float m_strength = 1.f;
        bool m_fixatePitch = true;
        bool m_fixateRoll = true;
        bool m_fixateYaw = true;
        bool m_enabled = true;
        bool m_applyLookAtTransform = true;

        // Framerate independence for strength application
        float m_accumulatedDeltaTime = 0.f;
        AZ::Transform m_lastDiscreteTM = AZ::Transform::Identity();
    };

}//namespace LmbrCentral
