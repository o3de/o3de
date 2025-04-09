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
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>

#include <PhysX/Configuration/PhysXConfiguration.h>

namespace PhysX
{
    namespace Internal
    {
        static constexpr const char* DefaultSceneNameFormat = "scene-%u";
    }
    //setup a test fixture with no default created scene
    class PhysXSystemFixture
        : public testing::Test
    {
    public:
        void SetUp() override
        {
            //create some configs to use in the tests.
            AzPhysics::SceneConfiguration config;
            for (int i = 0; i < NumScenes; i++)
            {
                config.m_sceneName = AZStd::string::format(Internal::DefaultSceneNameFormat, i);

                m_sceneConfigs.push_back(config);
            }
        }
        void TearDown() override
        {
            m_sceneConfigs.clear();

            TestUtils::ResetPhysXSystem();
        }

        size_t GetNumScenesInSystem() const
        {
            auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
            const AzPhysics::SceneList& sceneList = physicsSystem->GetAllScenes();

            size_t scenesNum = AZStd::count_if(sceneList.begin(), sceneList.end(),
                [](const auto& scenePtr){ return scenePtr != nullptr;});

            return scenesNum;
        }

        static constexpr AZ::u64 NumScenes = 10;
        AzPhysics::SceneConfigurationList m_sceneConfigs;
        PhysXSystemConfiguration m_systemConfig;
    };

    TEST_F(PhysXSystemFixture, ChangingSystemConfig_ShouldOnlySendEventIfChanged)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //setup configs
        PhysXSystemConfiguration preTestConfig;
        if (const auto* config = azdynamic_cast<const PhysXSystemConfiguration*>(physicsSystem->GetConfiguration()))
        {
            preTestConfig = *config;
        }
        
        PhysXSystemConfiguration modifiedConfig = preTestConfig;
        modifiedConfig.m_fixedTimestep = 0.0f;

        //create event handler
        int triggeredCount = 0;
        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler eventHandler(
            [&triggeredCount]([[maybe_unused]]const AzPhysics::SystemConfiguration* config)
            {
                triggeredCount++;
            }
        );

        //register handler
        physicsSystem->RegisterSystemConfigurationChangedEvent(eventHandler);

        //update config
        physicsSystem->UpdateConfiguration(&preTestConfig); //this should not signal the event
        EXPECT_EQ(triggeredCount, 0);
        physicsSystem->UpdateConfiguration(&modifiedConfig); //this should signal the event
        EXPECT_EQ(triggeredCount, 1);

        //reset state
        eventHandler.Disconnect();
        physicsSystem->UpdateConfiguration(&preTestConfig);
    }

    TEST_F(PhysXSystemFixture, AddScenes_createsAllScenesRequested)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_EQ(GetNumScenesInSystem(), 0); //there should be no scenes currently created

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //there should be the same number of scene handles as configs supplied
        EXPECT_EQ(sceneHandles.size(), m_sceneConfigs.size());

        //the returned handles should match the order of the supplied configs.
        for (int i = 0; i< m_sceneConfigs.size(); i++)
        {
            AzPhysics::Scene* scene = physicsSystem->GetScene(sceneHandles[i]);
            EXPECT_EQ(scene->GetConfiguration().m_sceneName, m_sceneConfigs[i].m_sceneName);
        }
    }

    TEST_F(PhysXSystemFixture, RemovedScene_IsRemoved)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_EQ(GetNumScenesInSystem(), 0); //there should be no scenes currently created

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //pick a scene handles to remove
         AzPhysics::SceneHandle removedSelection = sceneHandles[sceneHandles.size() / 2];
         physicsSystem->RemoveScene(removedSelection);
 
         EXPECT_EQ(physicsSystem->GetScene(removedSelection), nullptr);
    }

    TEST_F(PhysXSystemFixture, RemoveManyScenes_AllRemoved)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_EQ(GetNumScenesInSystem(), 0); //there should be no scenes currently created

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //select half of the scenes to be removed
        AzPhysics::SceneHandleList removedHandles;
        removedHandles.reserve(sceneHandles.size() / 2);
        for (int i = 0; i < sceneHandles.size(); i += 2)
        {
            removedHandles.emplace_back(sceneHandles[i]);
        }

        //remove all selected scene
        physicsSystem->RemoveScenes(removedHandles);

        //verify all removed handles return nullptr
        for (auto handle : removedHandles)
        {
            EXPECT_EQ(physicsSystem->GetScene(handle), nullptr);
        }
    }

    TEST_F(PhysXSystemFixture, RemovingScene_FreesSceneHandle_ForNextCreatedScene)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_EQ(GetNumScenesInSystem(), 0); //there should be no scenes currently created

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //pick a scene handles to remove
        AzPhysics::SceneHandle removedSelection = sceneHandles[sceneHandles.size() / 2];
        physicsSystem->RemoveScene(removedSelection);

        AzPhysics::SceneConfiguration newSceneConfig;
        newSceneConfig.m_sceneName = "NewScene";
        AzPhysics::SceneHandle newSceneHandle = physicsSystem->AddScene(newSceneConfig);

        //The old and new scene handle should share an index as the freed slot will be used
        EXPECT_EQ(AZStd::get<AzPhysics::HandleTypeIndex::Index>(removedSelection),
                  AZStd::get<AzPhysics::HandleTypeIndex::Index>(newSceneHandle));
    }

    TEST_F(PhysXSystemFixture, AddingScenes_PastLimitFails)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_EQ(GetNumScenesInSystem(), 0); //there should be no scenes currently created

        //generate the max number of scenes
        AzPhysics::SceneConfigurationList sceneConfigs;
        AzPhysics::SceneConfiguration config;
        for (AZ::u32 i = 0; i < AzPhysics::MaxNumberOfScenes; i++)
        {
            config.m_sceneName = AZStd::string::format(Internal::DefaultSceneNameFormat, i);
            sceneConfigs.push_back(config);
        }
        config.m_sceneName = "boom!";

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(sceneConfigs);
        //Verify we've added the max number and all are valid
        EXPECT_EQ(sceneHandles.size(), AzPhysics::MaxNumberOfScenes);
        for (auto& handle : sceneHandles)
        {
            EXPECT_TRUE(handle != AzPhysics::InvalidSceneHandle);
        }

        //Add the extra scene that should fail
        AzPhysics::SceneHandle failAddSceneHandle = physicsSystem->AddScene(config);
        //the handle should be invalid
        EXPECT_EQ(failAddSceneHandle, AzPhysics::InvalidSceneHandle);
    }

    TEST_F(PhysXSystemFixture, PrePostSimulateEvents_TriggerAsExpected)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        const AzPhysics::SystemConfiguration* config = physicsSystem->GetConfiguration();
       
        const float frameDeltaTime = AZStd::clamp(1.0f / 30.0f, 0.0f, config->m_maxTimestep);
        float expectedTickTime = AZStd::floorf(frameDeltaTime / config->m_fixedTimestep) * config->m_fixedTimestep;
        //init handlers
        int preSimEventCount = 0;
        int postSimEventCount = 0;
        AzPhysics::SystemEvents::OnPresimulateEvent::Handler preSimEvent(
            [&expectedTickTime, &preSimEventCount](float deltaTime)
            {
                EXPECT_NEAR(expectedTickTime, deltaTime, 0.001f);
                preSimEventCount++;
            });
        AzPhysics::SystemEvents::OnPostsimulateEvent::Handler postSimEvent(
            [&expectedTickTime, &postSimEventCount](float deltaTime)
            {
                EXPECT_NEAR(expectedTickTime, deltaTime, 0.001f);
                postSimEventCount++;
            });
        physicsSystem->RegisterPreSimulateEvent(preSimEvent);
        physicsSystem->RegisterPostSimulateEvent(postSimEvent);

        //run for 1 frame, handlers should be called once.
        physicsSystem->Simulate(frameDeltaTime);
        EXPECT_TRUE(preSimEventCount == 1);
        EXPECT_TRUE(postSimEventCount == 1);

        preSimEventCount = 0;
        postSimEventCount = 0;
        //run for 5 frames, handlers should be called 5 times
        constexpr const float numFrames = 5;
        float accumulatedTime = frameDeltaTime - config->m_fixedTimestep;
        for (int i = 0; i < numFrames; i++)
        {
            accumulatedTime += frameDeltaTime;
            expectedTickTime = AZStd::floorf(accumulatedTime / config->m_fixedTimestep) * config->m_fixedTimestep;
            physicsSystem->Simulate(frameDeltaTime);
            accumulatedTime -= expectedTickTime;
        }
        EXPECT_TRUE(preSimEventCount == numFrames);
        EXPECT_TRUE(postSimEventCount == numFrames);
    }

    TEST_F(PhysXSystemFixture, PreSimulateEvent_WithDeltaTime_GreaterThanMaxTimeStep_ShouldSendMaxTimeStep)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        const float fixedTimeStep = physicsSystem->GetConfiguration()->m_fixedTimestep;
        const float maxTimeStep = physicsSystem->GetConfiguration()->m_maxTimestep;
        const float frameDeltaTime = (maxTimeStep) * 2.0f; //create a delta time greater than the max time step.
        //this is the same logic as in PhysXSystem::Simualte
        float expectedTimeStep = AZ::GetClamp(frameDeltaTime, 0.0f, maxTimeStep);
        expectedTimeStep = AZStd::floorf(expectedTimeStep / fixedTimeStep) * fixedTimeStep;

        //init handler
        AzPhysics::SystemEvents::OnPresimulateEvent::Handler preSimEvent(
            [expectedTimeStep](float deltaTime)
            {
                EXPECT_NEAR(expectedTimeStep, deltaTime, 0.001f);
            });
        physicsSystem->RegisterPreSimulateEvent(preSimEvent);
        physicsSystem->Simulate(frameDeltaTime);
    }

    TEST_F(PhysXSystemFixture, GetHandle_ReturnsExpected)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //setup the expected handle to get.
        const int testSceneHandleIdx = PhysXSystemFixture::NumScenes / 2; //pick one in range
        const AzPhysics::SceneHandle expectedSceneHandle = sceneHandles[testSceneHandleIdx];

        //create name as the same way they're created in PhysXSystemFixture::Setup();
        const AZStd::string sceneName = AZStd::string::format(Internal::DefaultSceneNameFormat, testSceneHandleIdx);

        AzPhysics::SceneHandle sceneHandle = physicsSystem->GetSceneHandle(sceneName);
        EXPECT_EQ(sceneHandle, expectedSceneHandle);

        //ask for a scene not in the list, returns InvalidSceneHandle
        sceneHandle = physicsSystem->GetSceneHandle("ThisSceneIsNotHere");
        EXPECT_EQ(sceneHandle, AzPhysics::InvalidSceneHandle);
    }

    TEST_F(PhysXSystemFixture, AddRemoveScenes_InvokesEvents)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();

        //setup the add scene handler
        int addedCount = 0;
        AzPhysics::SystemEvents::OnSceneAddedEvent::Handler onAddedHandler([&addedCount]([[maybe_unused]]AzPhysics::SceneHandle sceneHandle)
            {
                addedCount++;
            });
        physicsSystem->RegisterSceneAddedEvent(onAddedHandler);

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);
        //the handler should be invoked the same number of times of the requested scenes.
        EXPECT_EQ(addedCount, m_sceneConfigs.size());

        //Setup the removed scene handler
        int removedCount = 0;
        AzPhysics::SystemEvents::OnSceneRemovedEvent::Handler onRemovedHandler([&removedCount, &sceneHandles, physicsSystem](AzPhysics::SceneHandle sceneHandle)
            {
                removedCount++;
                auto foundItr = AZStd::find(sceneHandles.begin(), sceneHandles.end(), sceneHandle);
                EXPECT_NE(foundItr, sceneHandles.end()); //the handle should be in the list.

                EXPECT_NE(physicsSystem->GetScene(sceneHandle), nullptr); //should return a valid scene.
            });
        physicsSystem->RegisterSceneRemovedEvent(onRemovedHandler);

        physicsSystem->RemoveScenes(sceneHandles);
        EXPECT_EQ(removedCount, m_sceneConfigs.size());
    }
}
