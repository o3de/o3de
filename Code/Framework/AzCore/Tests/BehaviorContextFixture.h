/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Interface/Interface.h>

namespace UnitTest
{
    // Basic fixture for managing a BehaviorContext
    class BehaviorContextFixture
        : public LeakDetectionFixture
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_behaviorContext = aznew AZ::BehaviorContext();

            AZ::ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
        }

        void TearDown() override
        {            
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            AZ::ComponentApplicationBus::Handler::BusDisconnect();

            // Just destroy everything before we complete the tear down.
            delete m_behaviorContext;
            m_behaviorContext = nullptr;

            LeakDetectionFixture::TearDown();
        }

        // ComponentApplicationBus
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override {}
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override {}
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override {}
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override {}
        void SignalEntityActivated(AZ::Entity*) override {}
        void SignalEntityDeactivated(AZ::Entity*) override {}
        bool AddEntity(AZ::Entity*) override { return true; }
        bool RemoveEntity(AZ::Entity*) override { return true; }
        bool DeleteEntity(const AZ::EntityId&) override { return true; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::SerializeContext* GetSerializeContext() override { return nullptr; }
        AZ::BehaviorContext*  GetBehaviorContext() override { return m_behaviorContext; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        ////

    protected:    
        AZ::BehaviorContext* m_behaviorContext;
    };
}
