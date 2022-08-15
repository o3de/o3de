/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockComponentApplication();
        ~MockComponentApplication();

        MOCK_METHOD1(FindEntity, AZ::Entity* (const AZ::EntityId&));
        MOCK_METHOD1(AddEntity, bool (AZ::Entity*));
        MOCK_METHOD0(Destroy, void ());
        MOCK_METHOD1(RegisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RegisterEntityAddedEventHandler, void(AZ::EntityAddedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityRemovedEventHandler, void(AZ::EntityRemovedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityActivatedEventHandler, void(AZ::EntityActivatedEvent::Handler&));
        MOCK_METHOD1(RegisterEntityDeactivatedEventHandler, void(AZ::EntityDeactivatedEvent::Handler&));
        MOCK_METHOD1(SignalEntityActivated, void(AZ::Entity*));
        MOCK_METHOD1(SignalEntityDeactivated, void(AZ::Entity*));
        MOCK_METHOD1(RemoveEntity, bool (AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool (const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string (const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void (const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_CONST_METHOD0(GetEngineRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };
} // namespace UnitTest
