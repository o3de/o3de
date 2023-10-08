/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
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
            m_motion->Destroy();
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
} // end namespace EMotionFX
