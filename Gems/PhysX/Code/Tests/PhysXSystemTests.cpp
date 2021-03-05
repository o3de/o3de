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
#include <Physics/PhysicsTests.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/PhysicsSystem.h>

#include <PhysX/Configuration/PhysXConfiguration.h>

namespace PhysX
{
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
                config.m_sceneName = AZStd::string::format("scene-%d", i);
                config.m_legacyId = AZ::Crc32(config.m_sceneName);

                m_sceneConfigs.push_back(config);
            }
        }
        void TearDown() override
        {
            m_sceneConfigs.clear();

            TestUtils::ResetPhysXSystem();
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
        EXPECT_TRUE(physicsSystem->GetAllScenes().size() == 0); //there should be no scenes currently created

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
        EXPECT_TRUE(physicsSystem->GetAllScenes().size() == 0); //there should be no scenes currently created

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
        EXPECT_TRUE(physicsSystem->GetAllScenes().size() == 0); //there should be no scenes currently created

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
        EXPECT_TRUE(physicsSystem->GetAllScenes().size() == 0); //there should be no scenes currently created

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(m_sceneConfigs);

        //pick a scene handles to remove
        AzPhysics::SceneHandle removedSelection = sceneHandles[sceneHandles.size() / 2];
        physicsSystem->RemoveScene(removedSelection);

        AzPhysics::SceneConfiguration newSceneConfig;
        newSceneConfig.m_sceneName = "NewScene";
        newSceneConfig.m_legacyId = AZ::Crc32(newSceneConfig.m_sceneName);
        AzPhysics::SceneHandle newSceneHandle = physicsSystem->AddScene(newSceneConfig);

        //The old and new scene handle should share an index as the freed slot will be used
        EXPECT_EQ(AZStd::get<AzPhysics::SceneHandleValues::Index>(removedSelection),
                  AZStd::get<AzPhysics::SceneHandleValues::Index>(newSceneHandle));
    }
#if AZ_TRAIT_DISABLE_FAILED_PHYSICS_TESTS
    TEST_F(PhysXSystemFixture, DISABLED_AddingScenes_PastLimitFails)
#else
    TEST_F(PhysXSystemFixture, AddingScenes_PastLimitFails)
#endif // AZ_TRAIT_DISABLE_FAILED_PHYSICS_TESTS
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        EXPECT_TRUE(physicsSystem->GetAllScenes().size() == 0); //there should be no scenes currently created

        //generate the max number of scenes
        AzPhysics::SceneConfigurationList sceneConfigs;
        AzPhysics::SceneConfiguration config;
        for (int i = 0; i < std::numeric_limits<AzPhysics::SceneIndex>::max(); i++)
        {
            config.m_sceneName = AZStd::string::format("scene-%d", i);
            config.m_legacyId = AZ::Crc32(config.m_sceneName);

            sceneConfigs.push_back(config);
        }
        config.m_sceneName = "boom!";
        config.m_legacyId = AZ::Crc32(config.m_sceneName);

        //add all scene configs
        AzPhysics::SceneHandleList sceneHandles = physicsSystem->AddScenes(sceneConfigs);
        //Verify we've added the max number and all are valid
        EXPECT_EQ(sceneHandles.size(), std::numeric_limits<AzPhysics::SceneIndex>::max());
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
            [&postSimEventCount]()
            {
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
}
