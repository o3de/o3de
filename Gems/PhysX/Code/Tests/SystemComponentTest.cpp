/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "PhysX_precompiled.h"

#include <gmock/gmock.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/World.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <PhysX/SystemComponentBus.h>
#include <Tests/EditorTestUtilities.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/Casts.h>
#include <AzFramework/Physics/Material.h>

#include <functional>

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityId.h>

namespace PhysXEditorTests
{
    class MockSystemNotificationBusHandler
        : public Physics::SystemNotificationBus::Handler
    {
    public:
        MockSystemNotificationBusHandler()
        {
            Physics::SystemNotificationBus::Handler::BusConnect();
        }

        ~MockSystemNotificationBusHandler()
        {
            Physics::SystemNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnWorldCreated, void(Physics::World*));
    };

    TEST_F(PhysXEditorFixture, SetDefaultSceneConfiguration_TriggersHandler)
    {
        bool handlerInvoked = false;
        AzPhysics::SystemEvents::OnDefaultSceneConfigurationChangedEvent::Handler defaultSceneConfigHandler(
            [&handlerInvoked]([[maybe_unused]] const AzPhysics::SceneConfiguration* config)
            {
                handlerInvoked = true;
            });

        // Initialize new configs with some non-default values.
        const AZ::Vector3 newGravity(2.f, 5.f, 7.f);
        const float newFixedTimeStep = 0.008f;
        const float newMaxTimeStep = 0.034f;

        AzPhysics::SceneConfiguration newConfiguration;

        newConfiguration.m_legacyConfiguration.m_gravity = newGravity;
        newConfiguration.m_legacyConfiguration.m_fixedTimeStep = newFixedTimeStep;
        newConfiguration.m_legacyConfiguration.m_maxTimeStep = newMaxTimeStep;

        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        physicsSystem->RegisterOnDefaultSceneConfigurationChangedEventHandler(defaultSceneConfigHandler);
        physicsSystem->UpdateDefaultSceneConfiguration(newConfiguration);

        EXPECT_TRUE(handlerInvoked);
    }

    TEST_F(PhysXEditorFixture, SystemNotificationBus_CreateWorld_HandlerReceivedOnCreateWorldNotification)
    {
        testing::StrictMock<MockSystemNotificationBusHandler> mockHandler;

        EXPECT_CALL(mockHandler, OnWorldCreated).Times(1);

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_legacyId = AZ::Crc32("SystemNotificationBusTestWorld");
            AzPhysics::SceneHandle sceneHandle = physicsSystem->AddScene(sceneConfiguration);
            if (AzPhysics::Scene* scene = physicsSystem->GetScene(sceneHandle))
            {
                auto world = scene->GetLegacyWorld();
            }
        }
    }
} // namespace PhysXEditorTests
