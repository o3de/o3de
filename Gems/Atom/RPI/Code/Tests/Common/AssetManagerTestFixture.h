/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectionManager.h>

namespace AZ
{
    class BehaviorContext;
    class SerializeContext;
}

namespace UnitTest
{
    /**
     * Unit test fixture for setting up an AssetManager
     */
    class AssetManagerTestFixture
        : public LeakDetectionFixture
        // Only used to provide the serialize context and behavior context for now
        , public AZ::ComponentApplicationBus::Handler
    {
    protected:
        void SetUp() override;
        void TearDown() override;
        
        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages. 
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return true; }
        bool RemoveEntity(AZ::Entity*) override { return true; }
        bool DeleteEntity(const AZ::EntityId&) override { return true; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::BehaviorContext* GetBehaviorContext() override;
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        AZ::SerializeContext* GetSerializeContext() override;
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<AZ::ReflectionManager> m_reflectionManager;
    };
} // namespace UnitTest

