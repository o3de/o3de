/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>

namespace AZ
{
    class ReflectContext;
    struct BehaviorArgument;
}

namespace AzPhysics::Automation
{
    //! Buses to expose Collision and Trigger event to Automation
    //! @{
    class AutomationCollisionNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~AutomationCollisionNotifications() = default;
        virtual void OnCollisionBegin(AZ::EntityId entityId, const AZStd::vector<AzPhysics::Contact>& contacts) = 0;
        virtual void OnCollisionPersist(AZ::EntityId entityId, const AZStd::vector<AzPhysics::Contact>& contacts) = 0;
        virtual void OnCollisionEnd(AZ::EntityId entityId) = 0;
    };
    using AutomationCollisionNotificationsBus = AZ::EBus<AutomationCollisionNotifications>;

    class AutomationTriggerNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~AutomationTriggerNotifications() = default;
        virtual void OnTriggerEnter(AZ::EntityId entityId) = 0;
        virtual void OnTriggerExit(AZ::EntityId entityId) = 0;
    };
    using AutomationTriggerNotificationsBus = AZ::EBus<AutomationTriggerNotifications>;
    //! @}

    //! Collision Event Handler for Automation
    //! @note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER macro as the signature
    //! needs to be changed for script canvas.
    class SimulatedBodyCollisionAutomationHandler
        : public AutomationCollisionNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::Automation::SimulatedBodyCollisionAutomationHandler, "{B0493CB7-9D20-44E1-B744-1419E54CAF67}", AZ::BehaviorEBusHandler);
        static void Reflect(AZ::ReflectContext* context);

        SimulatedBodyCollisionAutomationHandler();

        using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<
            decltype(&SimulatedBodyCollisionAutomationHandler::OnCollisionBegin),
            decltype(&SimulatedBodyCollisionAutomationHandler::OnCollisionPersist),
            decltype(&SimulatedBodyCollisionAutomationHandler::OnCollisionEnd)
        >;
    private:
        // AutomationCollisionNotificationsBus::Handler Interface
        void OnCollisionBegin([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZStd::vector<AzPhysics::Contact>& contacts) override {}
        void OnCollisionPersist([[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZStd::vector<AzPhysics::Contact>& contacts) override {}
        void OnCollisionEnd([[maybe_unused]] AZ::EntityId entityId) override {}

        enum
        {
            FN_OnCollisionBegin,
            FN_OnCollisionPersist,
            FN_OnCollisionEnd,
            FN_MAX
        };

        // AZ::BehaviorEBusHandler interface
        void Disconnect(AZ::BehaviorArgument* id = nullptr) override;
        bool Connect(AZ::BehaviorArgument* id = nullptr) override;
        bool IsConnected() override;
        bool IsConnectedId(AZ::BehaviorArgument* id) override;
        int GetFunctionIndex(const char* functionName) const override;

        void OnCollisionBeginEvent(const CollisionEvent& event);
        void OnCollisionPersistEvent(const CollisionEvent& event);
        void OnCollisionEndEvent(const CollisionEvent& event);

        AZ::EntityId m_connectedEntityId;
        SimulatedBodyEvents::OnCollisionBegin::Handler m_collisionBeginHandler;
        SimulatedBodyEvents::OnCollisionPersist::Handler m_collisionPersistHandler;
        SimulatedBodyEvents::OnCollisionEnd::Handler m_collisionEndHandler;
    };

    //! Trigger Event Handler for Automation
    //! @note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER macro as the signature
    //! needs to be changed for script canvas.
    class SimulatedBodyTriggerAutomationHandler
        : public AutomationTriggerNotificationsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::Automation::SimulatedBodyTriggerAutomationHandler, "{0BFA757E-F270-40D7-8543-B21260A987D0}", AZ::BehaviorEBusHandler);
        static void Reflect(AZ::ReflectContext* context);

        SimulatedBodyTriggerAutomationHandler();

        using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<
            decltype(&SimulatedBodyTriggerAutomationHandler::OnTriggerEnter),
            decltype(&SimulatedBodyTriggerAutomationHandler::OnTriggerExit)
        >;
    private:
        // AutomationTriggerNotificationsBus::Handler Interface
        void OnTriggerEnter([[maybe_unused]] AZ::EntityId entityId) override {}
        void OnTriggerExit([[maybe_unused]] AZ::EntityId entityId) override {}

        enum
        {
            FN_OnTriggerEnter,
            FN_OnTriggerExit,
            FN_MAX
        };

        // AZ::BehaviorEBusHandler interface
        void Disconnect(AZ::BehaviorArgument* id = nullptr) override;
        bool Connect(AZ::BehaviorArgument* id = nullptr) override;
        bool IsConnected() override;
        bool IsConnectedId(AZ::BehaviorArgument* id) override;
        int GetFunctionIndex(const char* functionName) const override;

        void OnTriggerEnterEvent(const TriggerEvent& event);
        void OnTriggerExitEvent(const TriggerEvent& event);

        AZ::EntityId m_connectedEntityId;
        SimulatedBodyEvents::OnTriggerEnter::Handler m_triggerEnterHandler;
        SimulatedBodyEvents::OnTriggerExit::Handler m_triggerExitHandler;
    };
}
