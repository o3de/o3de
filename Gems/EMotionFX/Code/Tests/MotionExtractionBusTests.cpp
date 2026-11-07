/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzCore/Component/TickBus.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/MotionExtractionBus.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/TestMotionAssets.h>

namespace EMotionFX
{

    class MotionExtractionTestBus
        : Integration::MotionExtractionRequestBus::Handler
    {
    public:
        MotionExtractionTestBus(AZ::EntityId entityId)
        {
            Integration::MotionExtractionRequestBus::Handler::BusConnect(entityId);
        }

        ~MotionExtractionTestBus()
        {
            Integration::MotionExtractionRequestBus::Handler::BusDisconnect();
        }

        MOCK_METHOD2(ExtractMotion, void(const AZ::Vector3&, float));
    };

    class MotionExtractionBusTests
        : public EntityComponentFixture
    {
    public:
        void SetUp() override
        {
            EntityComponentFixture::SetUp();
            m_entityId = AZ::EntityId(740216387);
            m_entity = AZStd::make_unique<AZ::Entity>(m_entityId);

            // Actor asset.
            AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
            AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));
            Integration::ActorComponent::Configuration actorConf;
            actorConf.m_actorAsset = actorAsset;

            m_entity->CreateComponent<AzFramework::TransformComponent>();
            auto actorComponent = m_entity->CreateComponent<Integration::ActorComponent>(&actorConf);
            auto animGraphComponent = m_entity->CreateComponent<Integration::AnimGraphComponent>();

            m_entity->Init();

            // Anim graph asset.
            AZ::Data::AssetId animGraphAssetId("{37629818-5166-4B96-83F5-5818B6A1F449}");
            animGraphComponent->SetAnimGraphAssetId(animGraphAssetId);
            AZ::Data::Asset<Integration::AnimGraphAsset> animGraphAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(animGraphAssetId);
            AZStd::unique_ptr<TwoMotionNodeAnimGraph> motionNodeAnimGraph = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
            m_animGraph = motionNodeAnimGraph.get();
            motionNodeAnimGraph.release();
            animGraphAsset.GetAs<Integration::AnimGraphAsset>()->SetData(m_animGraph);
            EXPECT_EQ(animGraphAsset.IsReady(), true) << "Anim graph asset is not ready yet.";
            animGraphComponent->OnAssetReady(animGraphAsset);

            // Motion set asset.
            AZ::Data::AssetId motionSetAssetId("{224BFF5F-D0AD-4216-9CEF-42F419CC6265}");
            animGraphComponent->SetMotionSetAssetId(motionSetAssetId);
            AZ::Data::Asset<Integration::MotionSetAsset> motionSetAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::MotionSetAsset>(motionSetAssetId);
            m_motionSet = aznew MotionSet("motionSet");
            Motion* motion = TestMotionAssets::GetJackWalkForward();
            AddMotionEntry(motion, "jack_walk_forward_aim_zup");

            m_animGraph->GetMotionNodeA()->AddMotionId("jack_walk_forward_aim_zup");

            motionSetAsset.GetAs<Integration::MotionSetAsset>()->SetData(new MotionSet());
            EXPECT_EQ(motionSetAsset.IsReady(), true) << "Motion set asset is not ready yet.";
            animGraphComponent->OnAssetReady(motionSetAsset);

            m_entity->Activate();

            // Actor component
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
            // note to maintainers
            // clearing a motionset calls "delete" on all the motions in it, so m_motion will already be destroyed
            delete m_motionSet;
            m_entity.reset();
            EntityComponentFixture::TearDown();
        }

    public:
        AZ::EntityId m_entityId;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        MotionSet* m_motionSet = nullptr;
        Motion* m_motion = nullptr;
        TwoMotionNodeAnimGraph* m_animGraph = nullptr;
    };

    TEST_F(MotionExtractionBusTests, ExtractMotionTests)
    {
        MotionExtractionTestBus testBus(m_entityId);
        
        const float timeDelta = 0.5f;
        const ActorManager* actorManager = GetEMotionFX().GetActorManager();
        const ActorInstance* actorInstance = actorManager->GetActorInstance(0);

        bool hasCustomMotionExtractionController = Integration::MotionExtractionRequestBus::FindFirstHandler(m_entityId) != nullptr;

        EXPECT_TRUE(hasCustomMotionExtractionController) << "MotionExtractionBus is not found.";

        AZ::Transform currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(currentTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);

        const AZ::Vector3 actorInstancePosition = actorInstance->GetWorldSpaceTransform().m_position;
        const AZ::Vector3 positionDelta = actorInstancePosition - currentTransform.GetTranslation();

        EXPECT_CALL(testBus, ExtractMotion(testing::_, testing::_));
        Integration::MotionExtractionRequestBus::Event(m_entityId, &Integration::MotionExtractionRequestBus::Events::ExtractMotion, positionDelta, timeDelta);
    }
    class RootMotionModifierTestHandler : public Integration::RootMotionModifierRequestBus::Handler
    {
    public:
        explicit RootMotionModifierTestHandler(AZ::EntityId entityId) { BusConnect(entityId); }
        ~RootMotionModifierTestHandler() override { BusDisconnect(); }
        bool ModifyRootMotion(const ActorInstance&, float simulationDeltaTime,
            const AZ::Transform& current, AZ::Transform& desired) override
        {
            ++m_calls;
            m_timestep = simulationDeltaTime;
            m_original = desired;
            desired.SetTranslation(current.GetTranslation() + AZ::Vector3(0.0f, 0.25f, 0.0f));
            return m_accept;
        }
        bool QueueRootMotionTranslation(const AZ::Vector3& delta, float timestep) override
        {
            ++m_queueCalls;
            m_queuedDelta = delta;
            m_queuedTime = timestep;
            return m_ownTranslation;
        }
        bool m_ownTranslation = false;
        int m_queueCalls = 0;
        AZ::Vector3 m_queuedDelta = AZ::Vector3::CreateZero();
        float m_queuedTime = 0.0f;
        bool m_accept = true;
        int m_calls = 0;
        float m_timestep = 0.0f;
        AZ::Transform m_original = AZ::Transform::CreateIdentity();
    };

    class RootMotionRecordingCharacter : public Physics::CharacterRequestBus::Handler
    {
    public:
        explicit RootMotionRecordingCharacter(AZ::EntityId entityId) { BusConnect(entityId); }
        ~RootMotionRecordingCharacter() override { BusDisconnect(); }
        AZ::Vector3 GetBasePosition() const override { return AZ::Vector3::CreateZero(); }
        void SetBasePosition(const AZ::Vector3&) override {}
        AZ::Vector3 GetCenterPosition() const override { return AZ::Vector3::CreateZero(); }
        float GetStepHeight() const override { return 0.0f; }
        void SetStepHeight(float) override {}
        AZ::Vector3 GetUpDirection() const override { return AZ::Vector3::CreateAxisZ(); }
        void SetUpDirection(const AZ::Vector3&) override {}
        float GetSlopeLimitDegrees() const override { return 0.0f; }
        void SetSlopeLimitDegrees(float) override {}
        float GetMaximumSpeed() const override { return 100.0f; }
        void SetMaximumSpeed(float) override {}
        AZ::Vector3 GetVelocity() const override { return m_velocity; }
        void AddVelocityForTick(const AZ::Vector3& velocity) override { m_velocity = velocity; ++m_calls; }
        void AddVelocityForPhysicsTimestep(const AZ::Vector3&) override {}
        bool IsPresent() const override { return true; }
        Physics::Character* GetCharacter() override { return nullptr; }
        AZ::Vector3 m_velocity = AZ::Vector3::CreateZero();
        int m_calls = 0;
    };

    TEST_F(MotionExtractionBusTests, ModifierRunsBeforePhysicsAndSubmitsMovementExactlyOnce)
    {
        ActorInstance* actor = nullptr;
        Integration::ActorComponentRequestBus::EventResult(actor, m_entityId,
            &Integration::ActorComponentRequests::GetActorInstance);
        ASSERT_NE(actor, nullptr);
        actor->GetActor()->AutoSetMotionExtractionNode();
        ASSERT_NE(actor->GetActor()->GetMotionExtractionNode(), nullptr);
        RootMotionModifierTestHandler modifier(m_entityId);
        RootMotionRecordingCharacter character(m_entityId);
        MotionExtractionTestBus customExtraction(m_entityId);
        EXPECT_CALL(customExtraction, ExtractMotion(testing::_, testing::_)).Times(0);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.25f, AZ::ScriptTimePoint());
        EXPECT_EQ(modifier.m_calls, 1);
        EXPECT_FLOAT_EQ(modifier.m_timestep, 0.25f);
        EXPECT_EQ(character.m_calls, 1);
        EXPECT_TRUE(character.m_velocity.IsClose(AZ::Vector3(0.0f, 1.0f, 0.0f), 0.0001f));
    }

    TEST_F(MotionExtractionBusTests, TranslationOwnerDefersMovementAndRebasesActorBeforePhysics)
    {
        ActorInstance* actor = nullptr;
        Integration::ActorComponentRequestBus::EventResult(actor, m_entityId,
            &Integration::ActorComponentRequests::GetActorInstance);
        ASSERT_NE(actor, nullptr);
        actor->GetActor()->AutoSetMotionExtractionNode();
        RootMotionModifierTestHandler modifier(m_entityId);
        modifier.m_ownTranslation = true;
        RootMotionRecordingCharacter character(m_entityId);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.0005f, AZ::ScriptTimePoint());
        EXPECT_EQ(modifier.m_queueCalls, 1);
        EXPECT_TRUE(modifier.m_queuedDelta.IsClose(AZ::Vector3(0.0f, 0.25f, 0.0f)));
        EXPECT_FLOAT_EQ(modifier.m_queuedTime, 0.0005f);
        EXPECT_EQ(character.m_calls, 0); // A parry can still discard the queued displacement.
        AZ::Vector3 actual = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(actual, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        EXPECT_TRUE(actor->GetWorldSpaceTransform().m_position.IsClose(actual));
    }

    TEST_F(MotionExtractionBusTests, ModifierAlsoRunsWithoutPhysicsController)
    {
        ActorInstance* actor = nullptr;
        Integration::ActorComponentRequestBus::EventResult(actor, m_entityId,
            &Integration::ActorComponentRequests::GetActorInstance);
        ASSERT_NE(actor, nullptr);
        actor->GetActor()->AutoSetMotionExtractionNode();
        RootMotionModifierTestHandler modifier(m_entityId);
        AZ::Transform before = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(before, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.25f, AZ::ScriptTimePoint());
        AZ::Transform after = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(after, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        EXPECT_EQ(modifier.m_calls, 1);
        EXPECT_TRUE(after.GetTranslation().IsClose(before.GetTranslation() + AZ::Vector3(0.0f, 0.25f, 0.0f), 0.0001f));
    }

    TEST_F(MotionExtractionBusTests, RejectedModifierRetainsOriginalMovement)
    {
        ActorInstance* actor = nullptr;
        Integration::ActorComponentRequestBus::EventResult(actor, m_entityId,
            &Integration::ActorComponentRequests::GetActorInstance);
        ASSERT_NE(actor, nullptr);
        actor->GetActor()->AutoSetMotionExtractionNode();
        RootMotionModifierTestHandler modifier(m_entityId);
        modifier.m_accept = false;
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.25f, AZ::ScriptTimePoint());
        AZ::Transform after = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(after, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        EXPECT_EQ(modifier.m_calls, 1);
        EXPECT_TRUE(after.GetTranslation().IsClose(modifier.m_original.GetTranslation(), 0.0001f));
        EXPECT_TRUE(after.GetRotation().IsClose(modifier.m_original.GetRotation(), 0.0001f));
    }
} // end namespace EMotionFX
