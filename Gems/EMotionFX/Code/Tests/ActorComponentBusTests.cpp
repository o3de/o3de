/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <EMotionFX/Source/Attachment.h>
#include <Integration/Components/ActorComponent.h>
#include <Include/Integration/ActorComponentBus.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/Printers.h>

namespace EMotionFX
{
    class ActorComponentNotificationTestBus
        : Integration::ActorComponentNotificationBus::Handler
    {
    public:
        ActorComponentNotificationTestBus(AZ::EntityId entityId)
        {
            Integration::ActorComponentNotificationBus::Handler::BusConnect(entityId);
        }

        ~ActorComponentNotificationTestBus()
        {
            Integration::ActorComponentNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnActorInstanceCreated, void(EMotionFX::ActorInstance*));
        MOCK_METHOD1(OnActorInstanceDestroyed, void(EMotionFX::ActorInstance*));
    };

    TEST_F(EntityComponentFixture, ActorComponentNotificationBusTest)
    {
        const AZ::EntityId id(740216387);

        ActorComponentNotificationTestBus testBus(id);
        EXPECT_CALL(testBus, OnActorInstanceCreated(testing::_)).Times(testing::AtLeast(1));
        EXPECT_CALL(testBus, OnActorInstanceDestroyed(testing::_)).Times(1);

        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>(id);
        entity->CreateComponent<AzFramework::TransformComponent>();
        Integration::ActorComponent::Configuration actorConf;
        Integration::ActorComponent* actorComponent = entity->CreateComponent<Integration::ActorComponent>(&actorConf);

        entity->Init();
        entity->Activate();

        const AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, ActorFactory::CreateAndInit<SimpleJointChainActor>(3));
        actorConf.m_actorAsset = actorAsset;
        actorComponent->SetActorAsset(actorConf.m_actorAsset);

        entity->Deactivate();
    }

    class ActorComponentRequestsFixture
        : public EntityComponentFixture
    {
    public:
        void SetUp() override
        {
            EntityComponentFixture::SetUp();

            const AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, ActorFactory::CreateAndInit<SimpleJointChainActor>(3));

            m_entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(740216387));
            m_transformComponent = m_entity->CreateComponent<AzFramework::TransformComponent>();

            Integration::ActorComponent::Configuration actorConfig;
            actorConfig.m_attachmentType = Integration::AttachmentType::SkinAttachment;
            actorConfig.m_actorAsset = actorAsset;
            m_actorComponent = m_entity->CreateComponent<Integration::ActorComponent>(&actorConfig);

            m_entity->Init();
            m_entity->Activate();

            m_actorComponent->SetActorAsset(actorAsset);
        }

        void TearDown() override
        {
            m_entity.reset();
            EntityComponentFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_entity;
        Integration::ActorComponent* m_actorComponent;
        AzFramework::TransformComponent* m_transformComponent;
    };

    TEST_F(ActorComponentRequestsFixture, GetActorInstance)
    {
        ActorInstance* gotActorInstance;
        Integration::ActorComponentRequestBus::EventResult(gotActorInstance, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetActorInstance);
        EXPECT_EQ(gotActorInstance, m_actorComponent->GetActorInstance());
    }

    TEST_F(ActorComponentRequestsFixture, GetNumJoints)
    {
        size_t gotNumJoints;
        Integration::ActorComponentRequestBus::EventResult(gotNumJoints, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetNumJoints);
        EXPECT_EQ(gotNumJoints, 3);
    }

    TEST_F(ActorComponentRequestsFixture, GetJointIndexByName)
    {
        size_t gotJointIndex;
        Integration::ActorComponentRequestBus::EventResult(gotJointIndex, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointIndexByName, "rootJoint");
        EXPECT_EQ(gotJointIndex, 0);
        Integration::ActorComponentRequestBus::EventResult(gotJointIndex, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointIndexByName, "joint1");
        EXPECT_EQ(gotJointIndex, 1);
        Integration::ActorComponentRequestBus::EventResult(gotJointIndex, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointIndexByName, "joint2");
        EXPECT_EQ(gotJointIndex, 2);
    }

    TEST_F(ActorComponentRequestsFixture, GetJointTransform)
    {
        AZ::Transform gotTransform;
        Integration::ActorComponentRequestBus::EventResult(gotTransform, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointTransform, 0, Integration::Space::LocalSpace);
        EXPECT_EQ(gotTransform.GetTranslation(), AZ::Vector3::CreateZero());
        Integration::ActorComponentRequestBus::EventResult(gotTransform, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointTransform, 1, Integration::Space::LocalSpace);
        EXPECT_EQ(gotTransform.GetTranslation(), AZ::Vector3::CreateAxisX(1.0f));
        Integration::ActorComponentRequestBus::EventResult(gotTransform, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetJointTransform, 2, Integration::Space::LocalSpace);
        EXPECT_EQ(gotTransform.GetTranslation(), AZ::Vector3::CreateAxisX(2.0f));
    }

#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    TEST_F(ActorComponentRequestsFixture, DISABLED_Attach_Detach_Entity)
#else
    TEST_F(ActorComponentRequestsFixture, Attach_Detach_Entity)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    {
        const AZ::Data::AssetId actorAssetId("{AD308159-879C-420E-B7D7-22E4A243F5A9}");
        AZ::Data::Asset<Integration::ActorAsset> targetActorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, ActorFactory::CreateAndInit<SimpleJointChainActor>(3, "parentActor"));

        auto targetEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(92484));

        AzFramework::TransformComponent* targetTransformComponent = targetEntity->CreateComponent<AzFramework::TransformComponent>();
        targetTransformComponent->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(9.0f, 24.0f, 84.0f)));

        Integration::ActorComponent::Configuration actorConf;
        actorConf.m_actorAsset = targetActorAsset;
        Integration::ActorComponent* targetActorComponent = targetEntity->CreateComponent<Integration::ActorComponent>(&actorConf);

        targetEntity->Init();
        targetEntity->Activate();

        targetActorComponent->SetActorAsset(targetActorAsset);

        Integration::ActorComponentRequestBus::Event(m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::AttachToEntity, targetEntity->GetId(), Integration::AttachmentType::SkinAttachment);

        EXPECT_EQ(
            m_transformComponent->GetWorldTM().GetTranslation(),
            targetTransformComponent->GetWorldTM().GetTranslation()
        );
        EXPECT_EQ(targetActorComponent->GetActorInstance()->GetNumAttachments(), 1);
        EXPECT_EQ(
            targetActorComponent->GetActorInstance()->GetAttachment(0)->GetAttachmentActorInstance(),
            m_actorComponent->GetActorInstance()
        );
        EXPECT_EQ(
            m_actorComponent->GetActorInstance()->GetAttachedTo(),
            targetActorComponent->GetActorInstance()
        );

        Integration::ActorComponentRequestBus::Event(m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::DetachFromEntity);

        EXPECT_EQ(
            m_transformComponent->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
        EXPECT_EQ(targetActorComponent->GetActorInstance()->GetNumAttachments(), 0);
        EXPECT_EQ(m_actorComponent->GetActorInstance()->GetAttachedTo(), nullptr);
    }

    TEST_F(ActorComponentRequestsFixture, GetSetRenderCharacter)
    {
        bool gotRenderCharacter = false;
        Integration::ActorComponentRequestBus::EventResult(gotRenderCharacter, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetRenderCharacter);

        EXPECT_TRUE(gotRenderCharacter);

        Integration::ActorComponentRequestBus::Event(m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::SetRenderCharacter, false);
        Integration::ActorComponentRequestBus::EventResult(gotRenderCharacter, m_entity->GetId(), &Integration::ActorComponentRequestBus::Events::GetRenderCharacter);

        EXPECT_FALSE(gotRenderCharacter);
    }
} // end namespace EMotionFX
