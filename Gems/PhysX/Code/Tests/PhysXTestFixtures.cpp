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

#include <PhysX_precompiled.h>
#include <Tests/PhysXTestFixtures.h>
#include <Tests/PhysXTestUtil.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/PhysicsScene.h>

//hook in the test environments for Gem::PhysX.Tests
#include <PhysXTestEnvironment.h>

#if HAVE_BENCHMARK

#include <Benchmarks/PhysXBenchmarksCommon.h>

AZ_UNIT_TEST_HOOK(new PhysX::TestEnvironment, PhysX::Benchmarks::PhysXBenchmarkEnvironment)

#else

AZ_UNIT_TEST_HOOK(new PhysX::TestEnvironment)

#endif // HAVE_BENCHMARK

namespace PhysX
{
    void PhysXDefaultWorldTest::SetUp() 
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_legacyId = Physics::DefaultPhysicsWorldId;
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            if (m_defaultScene = physicsSystem->GetScene(m_testSceneHandle))
            {
                m_defaultScene->GetLegacyWorld()->SetEventHandler(this);
            }
        }

        Physics::DefaultWorldBus::Handler::BusConnect();

        m_dummyTerrainComponentDescriptor = DummyTestTerrainComponent::CreateDescriptor();
    }

    void PhysXDefaultWorldTest::TearDown()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultScene = nullptr;
        m_dummyTerrainComponentDescriptor->ReleaseDescriptor();
        m_dummyTerrainComponentDescriptor = nullptr;

        //Clean up the Test scene
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        TestUtils::ResetPhysXSystem();
    }

    AZStd::shared_ptr<Physics::World> PhysXDefaultWorldTest::GetDefaultWorld()
    {
        return m_defaultScene->GetLegacyWorld();
    }

    void PhysXDefaultWorldTest::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerEnter, triggerEvent);
    }

    void PhysXDefaultWorldTest::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerExit, triggerEvent);
    }

    void PhysXDefaultWorldTest::OnCollisionBegin(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionBegin, event);
    }

    void PhysXDefaultWorldTest::OnCollisionPersist(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionPersist, event);
    }

    void PhysXDefaultWorldTest::OnCollisionEnd(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionEnd, event);
    }
}
