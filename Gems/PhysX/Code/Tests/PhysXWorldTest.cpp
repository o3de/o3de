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
#include <AzTest/AzTest.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    class WorldEventListener
        : public Physics::WorldNotificationBus::Handler

    {
    public:

        WorldEventListener(const char* worldId = "")
            : WorldEventListener(worldId, WorldNotifications::Default)
        {
        }

        WorldEventListener(const  char* worldId, int priority)
            : m_updateOrder(priority)
        {
            Physics::WorldNotificationBus::Handler::BusConnect(AZ::Crc32(worldId));
        }

        void Cleanup()
        {
            Physics::WorldNotificationBus::Handler::BusDisconnect();
        }

        void OnPrePhysicsSubtick(float fixedDeltaTime) override
        {
            if(m_onPreUpdate)
                m_onPreUpdate(fixedDeltaTime);
            m_preUpdates.push_back(fixedDeltaTime);
        }

        void OnPostPhysicsSubtick(float fixedDeltaTime) override
        {
            if (m_onPostUpdate)
                m_onPostUpdate(fixedDeltaTime); 
            m_postUpdates.push_back(fixedDeltaTime);
        }

        int GetPhysicsTickOrder() override
        {
            return m_updateOrder;
        }

        AZStd::vector<float> m_preUpdates;
        AZStd::vector<float> m_postUpdates;
        AZStd::function<void(float)> m_onPostUpdate;
        AZStd::function<void(float)> m_onPreUpdate;
        int m_updateOrder = WorldNotifications::Default;
    };

    class PhysXWorldTest
        : public::testing::Test
    {
    protected:
        using ScenePtr = AzPhysics::Scene*;

        AZStd::tuple<ScenePtr, WorldEventListener> CreateWorld(const char* worldId)
        {
            ScenePtr scene = nullptr;
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
                sceneConfiguration.m_legacyId = AZ::Crc32(worldId);
                AzPhysics::SceneHandle sceneHandle = physicsSystem->AddScene(sceneConfiguration);
                if (scene = physicsSystem->GetScene(sceneHandle))
                {
                    m_createdSceneHandles.emplace_back(sceneHandle);
                }
            }
            return AZStd::make_tuple(scene, WorldEventListener(worldId));
        }

        void TearDown() override
        {
            //Ensure any Scenes used have been created are removed
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScenes(m_createdSceneHandles);
            }
            m_createdSceneHandles.clear();
            TestUtils::ResetPhysXSystem();
        }

        AzPhysics::SceneHandleList m_createdSceneHandles;
    };

    TEST_F(PhysXWorldTest, OnPostUpdateTriggeredPerWorld)
    {
        constexpr const float deltaTime = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
        auto* phsyicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //Setup Scene 1
        ScenePtr scene1;
        WorldEventListener listener1;
        std::tie(scene1, listener1) = CreateWorld("scene1");

        // Tick the physics system, the fixed update should only run once.
        phsyicsSystem->Simulate(deltaTime);
        listener1.Cleanup(); //disconnect the handler

        //setup scene 2
        ScenePtr scene2;
        WorldEventListener listener2;
        std::tie(scene2, listener2) = CreateWorld("scene2");

        // Tick the physics system, the fixed update should run twice.
        phsyicsSystem->Simulate(deltaTime*2);

        listener2.Cleanup();
        // Then, we should receive correct amount of updates per world
        EXPECT_EQ(1, listener1.m_postUpdates.size());
        EXPECT_EQ(2, listener2.m_postUpdates.size());
    }

    TEST_F(PhysXWorldTest, OnPreUpdateTriggeredPerWorld)
    {
        constexpr const float deltaTime = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
        auto* phsyicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //Setup Scene 1
        ScenePtr scene1;
        WorldEventListener listener1;
        std::tie(scene1, listener1) = CreateWorld("scene1");

        // Tick the physics system, the fixed update should only run once.
        phsyicsSystem->Simulate(deltaTime);
        listener1.Cleanup(); //disconnect the handler

        //setup scene 2
        ScenePtr scene2;
        WorldEventListener listener2;
        std::tie(scene2, listener2) = CreateWorld("scene2");

        // Tick the physics system, the fixed update should run twice.
        phsyicsSystem->Simulate(deltaTime * 2);

        listener2.Cleanup();
        // Then, we should receive correct amount of updates per world
        EXPECT_EQ(1, listener1.m_preUpdates.size());
        EXPECT_EQ(2, listener2.m_preUpdates.size());
    }

    TEST_F(PhysXWorldTest, WorldNotificationBusOrdered)
    {
        // GIVEN there is a world with multiple listeners
        const char* worldId = "scene1";
        ScenePtr scene = nullptr;
        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        if (physicsSystem != nullptr)
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_legacyId = AZ::Crc32(worldId);
            sceneHandle = physicsSystem->AddScene(sceneConfiguration);
            scene = physicsSystem->GetScene(sceneHandle);
        }

        // Connect the buses in random-ish order.
        WorldEventListener listener1(worldId, Physics::WorldNotifications::Physics);
        WorldEventListener listener5(worldId, Physics::WorldNotifications::Default);
        WorldEventListener listener3(worldId, Physics::WorldNotifications::Components);
        WorldEventListener listener4(worldId, Physics::WorldNotifications::Scripting);
        WorldEventListener listener2(worldId, Physics::WorldNotifications::Animation);
        
        AZStd::vector<int> updateEvents;
        listener1.m_onPostUpdate = [&](float){
            updateEvents.push_back(listener1.m_updateOrder);
        };

        listener2.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener2.m_updateOrder);
        };

        listener3.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener3.m_updateOrder);
        };

        listener4.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener4.m_updateOrder);
        };

        listener5.m_onPostUpdate = [&](float) {
            updateEvents.push_back(listener5.m_updateOrder);
        };

        // WHEN the world is ticked.
        TestUtils::UpdateScene(scene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);

        listener1.Cleanup();
        listener2.Cleanup();
        listener3.Cleanup();
        listener4.Cleanup();
        listener5.Cleanup();

        // THEN all the listeners were updated in the correct order.
        EXPECT_EQ(5, updateEvents.size());
        EXPECT_TRUE(AZStd::is_sorted(updateEvents.begin(), updateEvents.end()));

        if (physicsSystem != nullptr)
        {
            scene = nullptr;
            physicsSystem->RemoveScene(sceneHandle);
            sceneHandle = AzPhysics::InvalidSceneHandle;
        }
    }
}
