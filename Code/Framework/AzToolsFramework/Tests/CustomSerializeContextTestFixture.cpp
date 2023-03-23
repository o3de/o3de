/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CustomSerializeContextTestFixture.h>

namespace UnitTest
{
    void CustomSerializeContextTestFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        AZ::ComponentApplicationBus::Handler::BusConnect();
    }

    void CustomSerializeContextTestFixture::TearDown()
    {
        AZ::ComponentApplicationBus::Handler::BusDisconnect();
        m_serializeContext.reset();
        LeakDetectionFixture::TearDown();
    }

    // ComponentApplicationBuss
    AZ::SerializeContext* CustomSerializeContextTestFixture::GetSerializeContext()
    {
        return m_serializeContext.get();
    }

    AZ::ComponentApplication* CustomSerializeContextTestFixture::GetApplication()
    {
        return nullptr;
    }

    void CustomSerializeContextTestFixture::RegisterComponentDescriptor(const AZ::ComponentDescriptor*)
    {
    }

    void CustomSerializeContextTestFixture::UnregisterComponentDescriptor(const AZ::ComponentDescriptor*)
    {
    }

    void CustomSerializeContextTestFixture::RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&)
    {
    }

    void CustomSerializeContextTestFixture::RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&)
    {
    }

    void CustomSerializeContextTestFixture::RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&)
    {
    }

    void CustomSerializeContextTestFixture::RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&)
    {
    }

    void CustomSerializeContextTestFixture::SignalEntityActivated(AZ::Entity*)
    {
    }

    void CustomSerializeContextTestFixture::SignalEntityDeactivated(AZ::Entity*)
    {
    }

    bool CustomSerializeContextTestFixture::AddEntity(AZ::Entity*)
    {
        return true;
    }

    bool CustomSerializeContextTestFixture::RemoveEntity(AZ::Entity*)
    {
        return true;
    }

    bool CustomSerializeContextTestFixture::DeleteEntity(const AZ::EntityId&)
    {
        return true;
    }

    AZ::Entity* CustomSerializeContextTestFixture::FindEntity(const AZ::EntityId&)
    {
        return nullptr;
    }

    AZ::BehaviorContext* CustomSerializeContextTestFixture::GetBehaviorContext()
    {
        return nullptr;
    }

    AZ::JsonRegistrationContext* CustomSerializeContextTestFixture::GetJsonRegistrationContext()
    {
        return nullptr;
    }

    const char* CustomSerializeContextTestFixture::GetEngineRoot() const
    {
        return nullptr;
    }

    const char* CustomSerializeContextTestFixture::GetExecutableFolder() const
    {
        return nullptr;
    }

    void CustomSerializeContextTestFixture::EnumerateEntities(const EntityCallback& /*callback*/)
    {
    }

    void CustomSerializeContextTestFixture::QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const
    {
    }
    ////
} // namespace UnitTest
