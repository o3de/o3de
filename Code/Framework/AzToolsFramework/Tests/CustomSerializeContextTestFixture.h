/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    class CustomSerializeContextTestFixture
        : public LeakDetectionFixture
        , AZ::ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override;
        void TearDown() override;

        // ComponentApplicationBus Overrides
        AZ::SerializeContext* GetSerializeContext() override;
        AZ::ComponentApplication* GetApplication() override;
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override;
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override;
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override;
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override;
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override;
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override;
        void SignalEntityActivated(AZ::Entity*) override;
        void SignalEntityDeactivated(AZ::Entity*) override;
        bool AddEntity(AZ::Entity*) override;
        bool RemoveEntity(AZ::Entity*) override;
        bool DeleteEntity(const AZ::EntityId&) override;
        AZ::Entity* FindEntity(const AZ::EntityId&) override;
        AZ::BehaviorContext* GetBehaviorContext() override;
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override;
        const char* GetEngineRoot() const override;
        const char* GetExecutableFolder() const override;
        void EnumerateEntities(const EntityCallback& /*callback*/) override;
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override;
        ////
    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };
} // namespace UnitTest
