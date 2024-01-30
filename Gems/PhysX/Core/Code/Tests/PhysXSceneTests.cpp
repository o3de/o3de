/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>

namespace PhysX
{
    //setup a test fixture with a scene named 'TestScene'
    class PhysXSceneFixture
        : public testing::Test
    {
    public:
        void SetUp() override
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration newSceneConfig;
                newSceneConfig.m_sceneName = "TestScene";
                m_testSceneHandle = physicsSystem->AddScene(newSceneConfig);
            }
        }
        void TearDown() override
        {
            //Cleanup any created scenes
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::SceneHandle m_testSceneHandle;
    };

    TEST_F(PhysXSceneFixture, ChangesToConfiguration_TriggersEvents)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //get the test scene, and the config.
        AzPhysics::Scene* scene = physicsSystem->GetScene(m_testSceneHandle);
        AzPhysics::SceneConfiguration newConfig = scene->GetConfiguration();

        //make a modification
        newConfig.m_gravity.SetX(42); 

        //setup the handler
        bool eventTriggered = false;
        AzPhysics::SceneEvents::OnSceneConfigurationChanged::Handler onConfigChanged(
            [&eventTriggered, newConfig, this](AzPhysics::SceneHandle sceneHandle, const AzPhysics::SceneConfiguration& config)
            {
                eventTriggered = true;
                //the event should come from the test scene
                EXPECT_EQ(sceneHandle, m_testSceneHandle);
                //the config should match the one provided
                EXPECT_EQ(config, newConfig);
            });
        scene->RegisterSceneConfigurationChangedEventHandler(onConfigChanged);

        //apply the config
        scene->UpdateConfiguration(newConfig);

        //The event should be triggered.
        EXPECT_TRUE(eventTriggered);
    }

    TEST_F(PhysXSceneFixture, CreateSimulatedBodies_WithSceneInterface)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //add a static rigid body
        AzPhysics::StaticRigidBodyConfiguration config;
        config.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &config);
        EXPECT_FALSE(simBodyHandle == AzPhysics::InvalidSimulatedBodyHandle);
    }

    TEST_F(PhysXSceneFixture, AddSimulatedBodies_returnsExpected)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AzPhysics::SimulatedBodyConfigurationList configs;

        //invalid scene handle returns empty
        AzPhysics::SimulatedBodyHandleList emptyBodies = sceneInterface->AddSimulatedBodies(AzPhysics::InvalidSceneHandle, configs);
        EXPECT_TRUE(emptyBodies.empty());
        emptyBodies = sceneInterface->AddSimulatedBodies(AzPhysics::SceneHandle(static_cast<AZ::u32>(2347892347890), AzPhysics::SceneIndex(7)), configs);
        EXPECT_TRUE(emptyBodies.empty());

        //add some rigid bodies
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        constexpr const int numberOfBodies = 100;
        for (int i = 0; i < numberOfBodies; i++)
        {
            const float xpos = 2.0f * static_cast<float>(i);
            AzPhysics::RigidBodyConfiguration* config = aznew AzPhysics::RigidBodyConfiguration();
            config->m_colliderAndShapeData = shapeColliderData;
            config->m_position = AZ::Vector3::CreateAxisX(xpos);
            configs.emplace_back(config);
        }
        //add 1 null entry into the list
        const int nullIndx = numberOfBodies / 3;
        configs.insert(configs.begin() + nullIndx, nullptr);

        //add the bodies
        AzPhysics::SimulatedBodyHandleList newBodies = sceneInterface->AddSimulatedBodies(m_testSceneHandle, configs);

        //should have a list of valid handles of the same size.
        EXPECT_TRUE(newBodies.size() == configs.size());
        for (int i = 0; i < numberOfBodies; i++)
        {
            if (i != nullIndx) //all other indexes should have valid handles.
            {
                EXPECT_FALSE(newBodies[i] == AzPhysics::InvalidSimulatedBodyHandle);
            }
            else //this is the index where a null config was sent, this should be invalid.
            {
                EXPECT_TRUE(newBodies[i] == AzPhysics::InvalidSimulatedBodyHandle);
            }
        }

        //cleanup
        for (auto* config : configs)
        {
            delete config;
        }
        configs.clear();
    }

    TEST_F(PhysXSceneFixture, GetSimulatedBodies_returnsExpected)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AzPhysics::SimulatedBodyConfigurationList configs;
        //add some rigid bodies
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        
        constexpr const int numberOfBodies = 100;
        for (int i = 0; i < numberOfBodies; i++)
        {
            const float xpos = 2.0f * static_cast<float>(i);
            AzPhysics::RigidBodyConfiguration* config = aznew AzPhysics::RigidBodyConfiguration();
            config->m_colliderAndShapeData = shapeColliderData;
            config->m_position = AZ::Vector3::CreateAxisX(xpos);
            configs.emplace_back(config);
        }

        //add the bodies
        AzPhysics::SimulatedBodyHandleList newBodies = sceneInterface->AddSimulatedBodies(m_testSceneHandle, configs);

        //invalid scene handle returns null
        AzPhysics::SimulatedBody* nullBody = sceneInterface->GetSimulatedBodyFromHandle(AzPhysics::InvalidSceneHandle, newBodies[0]);
        EXPECT_TRUE(nullBody == nullptr);
        nullBody = sceneInterface->GetSimulatedBodyFromHandle(AzPhysics::SceneHandle(static_cast<AZ::u32>(2347892347890), AzPhysics::SceneIndex(7)), newBodies[0]);
        EXPECT_TRUE(nullBody == nullptr);

        //invalid simulated body handle returns null
        nullBody = sceneInterface->GetSimulatedBodyFromHandle(AzPhysics::InvalidSceneHandle, AzPhysics::InvalidSimulatedBodyHandle);
        EXPECT_TRUE(nullBody == nullptr);
        nullBody = sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, AzPhysics::SimulatedBodyHandle(1347892348, 9));
        EXPECT_TRUE(nullBody == nullptr);

        //get 1 simulated body, should not be null.
        AzPhysics::SimulatedBody* bodyPtr = sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, newBodies[0]);
        EXPECT_TRUE(bodyPtr != nullptr);

        //request by list
        AzPhysics::SimulatedBodyList bodies = sceneInterface->GetSimulatedBodiesFromHandle(m_testSceneHandle, newBodies);
        //lists should be the same size
        EXPECT_EQ(bodies.size(), newBodies.size());
        //none should be null
        for (auto* body : bodies)
        {
            EXPECT_TRUE(body != nullptr);
        }

        //cleanup
        for (auto* config : configs)
        {
            delete config;
        }
        configs.clear();
    }

    TEST_F(PhysXSceneFixture, RemovedSimulatedBody_IsRemoved)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //add a simulated body
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        AzPhysics::StaticRigidBodyConfiguration config;
        config.m_colliderAndShapeData = shapeColliderData;
        AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &config);

        //remove the body
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, simBodyHandle);

        //GetSimulatedBodyFromHandle should return nullptr
        EXPECT_EQ(sceneInterface->GetSimulatedBodyFromHandle(m_testSceneHandle, simBodyHandle), nullptr);
    }

    TEST_F(PhysXSceneFixture, RemovedSimulatedBody_FreesSimulatedBodyIndex_ForNextCreated)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //add a few simulated body
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        AzPhysics::StaticRigidBodyConfiguration config;
        config.m_colliderAndShapeData = shapeColliderData;

        AzPhysics::SimulatedBodyHandleList simBodyHandles;
        constexpr const int numBodies = 10;
        for (int i = 0; i < numBodies; i++)
        {
            simBodyHandles.emplace_back(sceneInterface->AddSimulatedBody(m_testSceneHandle, &config));
        }

        //select 1 to remove
        AzPhysics::SimulatedBodyHandle removedSelection = simBodyHandles[simBodyHandles.size() / 2];
        const AzPhysics::SimulatedBodyIndex removedIndex = AZStd::get<AzPhysics::HandleTypeIndex::Index>(removedSelection);
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, removedSelection);

        // The removedSelection handle should be set to invalid in RemoveSimulatedBody
        EXPECT_EQ(removedSelection, AzPhysics::InvalidSimulatedBodyHandle);

        //add a new one.
        AzPhysics::SimulatedBodyHandle newSimBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &config);

        //The old and new handle should share an index as the freed slot will be used
        EXPECT_EQ(removedIndex,
                  AZStd::get<AzPhysics::HandleTypeIndex::Index>(newSimBodyHandle));
    }

    TEST_F(PhysXSceneFixture, AddRemoveSimulatedBodies_SendEvents)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //setup the add/remove handlers
        bool addTriggered = false;
        AzPhysics::SimulatedBodyHandle addEventSimBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneEvents::OnSimulationBodyAdded::Handler addedEvent(
            [&addEventSimBodyHandle, &addTriggered, this](AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle simBodyHandle)
            {
                addTriggered = true;
                addEventSimBodyHandle = simBodyHandle;
                EXPECT_EQ(sceneHandle, m_testSceneHandle);
            });

        bool removedTriggered = false;
        AzPhysics::SimulatedBodyHandle removeEventSimBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneEvents::OnSimulationBodyRemoved::Handler removedEvent(
            [&removeEventSimBodyHandle, &removedTriggered, this](AzPhysics::SceneHandle sceneHandle, AzPhysics::SimulatedBodyHandle simBodyHandle)
            {
                removedTriggered = true;
                removeEventSimBodyHandle = simBodyHandle;
                EXPECT_EQ(sceneHandle, m_testSceneHandle);
            });

        //register the handlers
        sceneInterface->RegisterSimulationBodyAddedHandler(m_testSceneHandle, addedEvent);
        sceneInterface->RegisterSimulationBodyRemovedHandler(m_testSceneHandle, removedEvent);

        //add a simulated body
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));
        AzPhysics::StaticRigidBodyConfiguration config;
        config.m_colliderAndShapeData = shapeColliderData;
        AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &config);

        EXPECT_TRUE(addTriggered);
        EXPECT_EQ(simBodyHandle, addEventSimBodyHandle);

        //remove the body
        const AzPhysics::SimulatedBodyHandle removedHandle = simBodyHandle; //copy the handle as RemoveSimulatedBody will mark it invalid.
        sceneInterface->RemoveSimulatedBody(m_testSceneHandle, simBodyHandle);
        EXPECT_TRUE(removedTriggered);
        EXPECT_EQ(removedHandle, removeEventSimBodyHandle);
    }

    TEST_F(PhysXSceneFixture, StartFinishSimulationEvents_triggerAsExpected)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        //add another scene for testing that only the registered scene sends the event + able to reuse 1 event handler for both scenes.
        AzPhysics::SceneConfiguration newSceneConfig;
        newSceneConfig.m_sceneName = "SecondTestScene";
        AzPhysics::SceneHandle secondTestSceneHandle = physicsSystem->AddScene(newSceneConfig);

        //struct to hold data for handlers
        struct EventTriggerData
        {
            int startCount = 0;
            int finishCount = 0;
            AzPhysics::SceneHandle m_sceneHandle = AzPhysics::InvalidSceneHandle;
        };

        // setup start/finish handlers that will connect to the test scene.
        EventTriggerData testScenedata;
        testScenedata.m_sceneHandle = m_testSceneHandle;
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler startTestSceneHandler(
            [&testScenedata](AzPhysics::SceneHandle sceneHandle, float fixedDeltaTime)
            {
                testScenedata.startCount++;
                EXPECT_EQ(sceneHandle, testScenedata.m_sceneHandle);
                EXPECT_NEAR(AzPhysics::SystemConfiguration::DefaultFixedTimestep, fixedDeltaTime, 0.001f);
            });

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler finishTestSceneHandler(
            [&testScenedata](AzPhysics::SceneHandle sceneHandle, [[maybe_unused]] float fixedDeltatime)
            {
                testScenedata.finishCount++;
                EXPECT_EQ(sceneHandle, testScenedata.m_sceneHandle);
            });
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, startTestSceneHandler);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, finishTestSceneHandler);

        // setup start/finish handlers that will connect to the second test scene.
        EventTriggerData secondTestScenedata;
        secondTestScenedata.m_sceneHandle = secondTestSceneHandle;
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler startSecondTestSceneHandler(
            [&secondTestScenedata](AzPhysics::SceneHandle sceneHandle, float fixedDeltaTime)
            {
                secondTestScenedata.startCount++;
                EXPECT_EQ(sceneHandle, secondTestScenedata.m_sceneHandle);
                EXPECT_NEAR(AzPhysics::SystemConfiguration::DefaultFixedTimestep, fixedDeltaTime, 0.001f);
            });

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler finishSecondTestSceneHandler(
            [&secondTestScenedata](AzPhysics::SceneHandle sceneHandle, [[maybe_unused]] float fixedDeltatime)
            {
                secondTestScenedata.finishCount++;
                EXPECT_EQ(sceneHandle, secondTestScenedata.m_sceneHandle);
            });
        sceneInterface->RegisterSceneSimulationStartHandler(secondTestSceneHandle, startSecondTestSceneHandler);
        sceneInterface->RegisterSceneSimulationFinishHandler(secondTestSceneHandle, finishSecondTestSceneHandler);

        //update the system to trigger the events
        physicsSystem->Simulate(AzPhysics::SystemConfiguration::DefaultFixedTimestep);

        //handlers should trigger once
        EXPECT_EQ(testScenedata.startCount, 1);
        EXPECT_EQ(testScenedata.finishCount, 1);
        EXPECT_EQ(secondTestScenedata.startCount, 1);
        EXPECT_EQ(secondTestScenedata.finishCount, 1);

        //clean up
        physicsSystem->RemoveScene(secondTestSceneHandle);
    }

    TEST_F(PhysXSceneFixture, StartFinishSimulationEvents_triggerInCorrectOrder)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        AZStd::vector<AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority> orderedEventTriggersStart;
        AZStd::vector<AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority> orderedEventTriggersFinish;

        //setup the start simulate handlers
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler physicsStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime )
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler animationStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation));
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler componentsStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components));
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler scriptingStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Scripting);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Scripting));
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler audioStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Audio);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Audio));
        AzPhysics::SceneEvents::OnSceneSimulationStartHandler defaultStartEvent(
            [&orderedEventTriggersStart](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltaTime)
            {
                orderedEventTriggersStart.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Default);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Default));

        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, physicsStartEvent);
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, animationStartEvent);
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, componentsStartEvent);
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, scriptingStartEvent);
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, audioStartEvent);
        sceneInterface->RegisterSceneSimulationStartHandler(m_testSceneHandle, defaultStartEvent);

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler physicsFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Physics));
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler animationFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation));
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler componentsFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components));
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler scriptingFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Scripting);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Scripting));
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler audioFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Audio);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Audio));
        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler defaultFinishEvent(
            [&orderedEventTriggersFinish](
                [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
                [[maybe_unused]] float fixedDeltatime)
            {
                orderedEventTriggersFinish.push_back(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Default);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Default));

        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, physicsFinishEvent);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, animationFinishEvent);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, componentsFinishEvent);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, scriptingFinishEvent);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, audioFinishEvent);
        sceneInterface->RegisterSceneSimulationFinishHandler(m_testSceneHandle, defaultFinishEvent);

        //trigger the events
        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);

        //all events should trigger
        EXPECT_EQ(6, orderedEventTriggersStart.size());
        EXPECT_EQ(6, orderedEventTriggersFinish.size());

        //is in order (sorted from Largest to smallest)
        auto compareOp = [](const AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority& lhs,
            const AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority& rhs) -> bool
        {
            return lhs > rhs;
        };
        EXPECT_TRUE(AZStd::is_sorted(orderedEventTriggersStart.begin(), orderedEventTriggersStart.end(), compareOp));
        EXPECT_TRUE(AZStd::is_sorted(orderedEventTriggersFinish.begin(), orderedEventTriggersFinish.end(), compareOp));
    }

    TEST_F(PhysXSceneFixture, ChangeGravity_sendsNotification)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        const AZ::Vector3 changedGravity(9.81f, 0.0f, 0.0f);
        bool eventTriggered = false;

        //setup handler
        AzPhysics::SceneEvents::OnSceneGravityChangedEvent::Handler onGravityChangedHandler(
            [this, changedGravity, &eventTriggered](AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& newGravity)
            {
                eventTriggered = true;

                // scene handles should match
                EXPECT_TRUE(sceneHandle == m_testSceneHandle);

                // the new gravity value should be close to the requested gravity
                EXPECT_TRUE(changedGravity.IsClose(newGravity));
            });
        sceneInterface->RegisterSceneGravityChangedEvent(m_testSceneHandle, onGravityChangedHandler);

        //update the gravity
        sceneInterface->SetGravity(m_testSceneHandle, changedGravity);

        EXPECT_TRUE(eventTriggered);
    }

    class PhysXSceneActiveSimulatedBodiesFixture
        : public testing::Test
    {
    public:
        void SetUp() override
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration newSceneConfig;
                newSceneConfig.m_sceneName = "TestScene";
                newSceneConfig.m_enableActiveActors = true;
                m_testSceneHandle = physicsSystem->AddScene(newSceneConfig);
            }
        }
        void TearDown() override
        {
            //Cleanup any created scenes
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        }

        AzPhysics::SceneHandle m_testSceneHandle;
    };

    TEST_F(PhysXSceneActiveSimulatedBodiesFixture, SceneActiveSimulatedBodies_CorrectlyReported)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        // setup shape config
        AzPhysics::ShapeColliderPair shapeColliderData(
            AZStd::make_shared<Physics::ColliderConfiguration>(),
            AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3::CreateOne()));

        // add a static simulated body - this is not expected to be reported as an active actor
        AzPhysics::StaticRigidBodyConfiguration staticConfig;
        staticConfig.m_colliderAndShapeData = shapeColliderData;
        sceneInterface->AddSimulatedBody(m_testSceneHandle, &staticConfig);

        // add a rigid body - this is expect to be reported as an active actor
        AzPhysics::RigidBodyConfiguration rigidConfig;
        rigidConfig.m_colliderAndShapeData = shapeColliderData;
        AzPhysics::SimulatedBodyHandle rigidSphereHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &rigidConfig);

        // create + register the active handler
        bool handlerTriggered = false;
        AzPhysics::SceneEvents::OnSceneActiveSimulatedBodiesEvent::Handler activeActorsHandler(
            [rigidSphereHandle, &handlerTriggered, this](AzPhysics::SceneHandle sceneHandle,
                const AzPhysics::SimulatedBodyHandleList& activeBodyList, float deltaTime)
            {
                handlerTriggered = true;
                //the scene handles should match
                EXPECT_TRUE(m_testSceneHandle == sceneHandle);

                //there should only be 1 entry and it should be the rigidSphereHandler
                EXPECT_TRUE(activeBodyList.size() == 1);
                EXPECT_TRUE(activeBodyList[0] == rigidSphereHandle);

                EXPECT_TRUE(AZ::IsClose(deltaTime, AzPhysics::SystemConfiguration::DefaultFixedTimestep));
            });
        sceneInterface->RegisterSceneActiveSimulatedBodiesHandler(m_testSceneHandle, activeActorsHandler);

        // run physics to trigger the event
        TestUtils::UpdateScene(m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);

        EXPECT_TRUE(handlerTriggered);
    }
}
