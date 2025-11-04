/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Integration/Components/SimpleMotionComponent.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>

namespace EMotionFX
{
    class SimpleMotionComponentBusTests
        : public EntityComponentFixture
    {
    public:
        void SetUp() override
        {
            EntityComponentFixture::SetUp();

            m_entity = AZStd::make_unique<AZ::Entity>();
            m_entityId = AZ::EntityId(740216387);
            m_entity->SetId(m_entityId);

            // Actor asset.
            AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
            AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));
            Integration::ActorComponent::Configuration actorConf;
            actorConf.m_actorAsset = actorAsset;

            m_entity->CreateComponent<AzFramework::TransformComponent>();
            auto actorComponent = m_entity->CreateComponent<Integration::ActorComponent>(&actorConf);
            m_simpleMotionComponent = m_entity->CreateComponent<Integration::SimpleMotionComponent>();

            m_entity->Init();
            
            // Motion asset.
            m_motionAssetId = AZ::Data::AssetId("{F3CDBB53-D793-449F-A086-2821680E3C38}");
            m_simpleMotionComponent->SetMotionAssetId(m_motionAssetId);
            AZ::Data::Asset<Integration::MotionAsset> motionAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::MotionAsset>(m_motionAssetId);
            m_motionSet = aznew MotionSet("motionSet");
            Motion* motion = TestMotionAssets::GetJackWalkForward();
            AddMotionEntry(motion, "jack_walk_forward_aim_zup");
            motionAsset.GetAs<Integration::MotionAsset>()->SetData(motion);
            m_simpleMotionComponent->OnAssetReady(motionAsset);

            // Actor component
            m_entity->Activate();
            actorComponent->SetActorAsset(actorAsset);
        }

        void AddMotionEntry(Motion* motion, const AZStd::string& motionId)
        {
            m_motion = motion;
            EMotionFX::MotionSet::MotionEntry* newMotionEntry = aznew EMotionFX::MotionSet::MotionEntry();
            newMotionEntry->SetMotion(m_motion);
            m_motionSet->AddMotionEntry(newMotionEntry);
            m_motionSet->SetMotionEntryId(newMotionEntry, motionId);
        }

        void TearDown() override
        {
            m_motionSet->Clear();
            // m_motion->Destroy(); // Will be destroyed through the m_entity destructor
            delete m_motionSet;
            m_entity.reset();
            EntityComponentFixture::TearDown();
        }

        AZ::Data::AssetId m_motionAssetId;
        AZ::EntityId m_entityId;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        Integration::SimpleMotionComponent* m_simpleMotionComponent = nullptr;
        MotionSet* m_motionSet = nullptr;
        Motion* m_motion = nullptr;
    };

    // Test GetMotion
    TEST_F(SimpleMotionComponentBusTests, GetMotionTest)
    {
        AZ::Data::AssetId motionAssetId;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            motionAssetId,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetMotion
        );
        EXPECT_EQ(m_motionAssetId, motionAssetId);
    }

    // Test LoopMotion and GetLoopMotion
    TEST_F(SimpleMotionComponentBusTests, LoopMotionTest)
    {
        bool loopMotion;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            loopMotion,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetLoopMotion
        );
        EXPECT_FALSE(loopMotion);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::LoopMotion,
            true
        );

        Integration::SimpleMotionComponentRequestBus::EventResult(
            loopMotion,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetLoopMotion
        );
        EXPECT_TRUE(loopMotion);
    }

    // Test SetPlaySpeed and GetPlaySpeed
    TEST_F(SimpleMotionComponentBusTests, PlaySpeedTest)
    {
        float playSpeed;
        const float defaultPlaySpeed = 1.0f;
        const float expectedPlaySpeed = 2.0f;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            playSpeed,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetPlaySpeed
        );
        EXPECT_EQ(playSpeed, defaultPlaySpeed);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::SetPlaySpeed,
            expectedPlaySpeed
        );

        Integration::SimpleMotionComponentRequestBus::EventResult(
            playSpeed,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetPlaySpeed
        );
        EXPECT_EQ(playSpeed, expectedPlaySpeed);
    }

    // Test GetPlayTime and PlayTime
    TEST_F(SimpleMotionComponentBusTests, PlayTimeTest)
    {
        float playTime;
        const float defaultPlayTime = 0.0f;
        const float expectedPlayTime = 1.5f;
        const float err_margin = 0.1f;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            playTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetPlayTime
        );
        EXPECT_EQ(playTime, defaultPlayTime);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::PlayTime,
            expectedPlayTime
        );

        Integration::SimpleMotionComponentRequestBus::EventResult(
            playTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetPlayTime
        );
        EXPECT_NEAR(playTime, expectedPlayTime, err_margin);
    }

    // Test BlendInTime and GetBlendInTime
    TEST_F(SimpleMotionComponentBusTests, BlendInTimeTest)
    {
        float blendInTime;
        const float defaultBlendInTime = 0.0f;
        const float expectedBlendInTime = 1.0f;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            blendInTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetBlendInTime
        );
        EXPECT_EQ(blendInTime, defaultBlendInTime);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::BlendInTime,
            expectedBlendInTime
        );
        Integration::SimpleMotionComponentRequestBus::EventResult(
            blendInTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetBlendInTime
        );
        EXPECT_EQ(blendInTime, expectedBlendInTime);
    }

    // Test BlendInTime and GetBlendInTime
    TEST_F(SimpleMotionComponentBusTests, BlendOutTimeTest)
    {
        float blendOutTime;
        const float defaultBlendOutTime = 0.0f;
        const float expectedBlendOutTime = 1.0f;
        Integration::SimpleMotionComponentRequestBus::EventResult(
            blendOutTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetBlendOutTime
        );
        EXPECT_EQ(blendOutTime, defaultBlendOutTime);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::BlendOutTime,
            expectedBlendOutTime
        );
        Integration::SimpleMotionComponentRequestBus::EventResult(
            blendOutTime,
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::GetBlendOutTime
        );
        EXPECT_EQ(blendOutTime, expectedBlendOutTime);
    }

    // Test PlayMotion
    TEST_F(SimpleMotionComponentBusTests, PlayMotionTest)
    {
        const MotionInstance* motionInstance = m_simpleMotionComponent->GetMotionInstance();
        EXPECT_NE(motionInstance, nullptr);
        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::PlayMotion
        );

        const MotionInstance* motionInstanceAfterPlayMotion = m_simpleMotionComponent->GetMotionInstance();
        EXPECT_NE(motionInstanceAfterPlayMotion, nullptr);
    }

    // Test MirrorMotion
    TEST_F(SimpleMotionComponentBusTests, MirrorMotionTest)
    {
        const MotionInstance* motionInstance = m_simpleMotionComponent->GetMotionInstance();
        bool mirrorMotion = motionInstance->GetMirrorMotion();
        EXPECT_FALSE(mirrorMotion);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::MirrorMotion,
            true
        );
        mirrorMotion = motionInstance->GetMirrorMotion();
        EXPECT_TRUE(mirrorMotion);
    }

    // Test RetargetingMotion
    TEST_F(SimpleMotionComponentBusTests, RetargetingMotionTest)
    {
        const MotionInstance* motionInstance = m_simpleMotionComponent->GetMotionInstance();
        bool retargetMotion = motionInstance->GetRetargetingEnabled();
        EXPECT_FALSE(retargetMotion);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::RetargetMotion,
            true
        );
        retargetMotion = motionInstance->GetRetargetingEnabled();
        EXPECT_TRUE(retargetMotion);
    }

    // Test ReverseMotion
    TEST_F(SimpleMotionComponentBusTests, ReverseMotionTest)
    {
        const MotionInstance* motionInstance = m_simpleMotionComponent->GetMotionInstance();
        EMotionFX::EPlayMode reverseMotion = motionInstance->GetPlayMode();
        EXPECT_EQ(reverseMotion, EMotionFX::EPlayMode::PLAYMODE_FORWARD);

        Integration::SimpleMotionComponentRequestBus::Event(
            m_entityId,
            &Integration::SimpleMotionComponentRequestBus::Events::ReverseMotion,
            true
        );
        reverseMotion = motionInstance->GetPlayMode();
        EXPECT_EQ(reverseMotion, EMotionFX::EPlayMode::PLAYMODE_BACKWARD);
    }
} // end namespace EMotionFX
