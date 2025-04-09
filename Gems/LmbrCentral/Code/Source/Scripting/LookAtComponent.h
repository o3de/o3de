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
        
        //! Set the target entity to look at
        virtual void SetTarget([[maybe_unused]] AZ::EntityId targetEntity) {}

        //! Set the target position to look at
        virtual void SetTargetPosition([[maybe_unused]] const AZ::Vector3& position) {}

        //! Set the reference forward axis
        virtual void SetAxis([[maybe_unused]] AZ::Transform::Axis axis = AZ::Transform::Axis::ZPositive) {}
    };

    using LookAtComponentRequestBus = AZ::EBus<LookAtComponentRequests>;

    class LookAtComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        //! Notifies you that the target has changed
        virtual void OnTargetChanged(AZ::EntityId) { }
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
        void SetTarget(AZ::EntityId targetEntity) override;
        void SetTargetPosition(const AZ::Vector3& targetPosition) override;
        void SetAxis(AZ::Transform::Axis axis) override;
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
        void RecalculateTransform();

        // Serialized data
        AZ::EntityId m_targetId;
        AZ::Vector3  m_targetPosition;

        AZ::Transform::Axis m_forwardAxis;
    };

}//namespace LmbrCentral
