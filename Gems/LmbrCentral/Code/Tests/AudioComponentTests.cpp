/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>

#include <Audio/AudioMultiPositionComponent.h>
#include <Audio/AudioProxyComponent.h>
#include <Audio/AudioTriggerComponent.h>


namespace UnitTest
{
    class AudioMultiPositionComponentTests
        : public LeakDetectionFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_audioProxyComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_audioTriggerComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_audioMultiPosComponentDescriptor;

    public:
        AudioMultiPositionComponentTests()
            : LeakDetectionFixture()
        {}

        AZStd::size_t GetNumEntityRefs(LmbrCentral::AudioMultiPositionComponent* audioComponent)
        {
            return audioComponent->GetNumEntityRefs();
        }

    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_audioProxyComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::AudioProxyComponent::CreateDescriptor());
            m_audioTriggerComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::AudioTriggerComponent::CreateDescriptor());
            m_audioMultiPosComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::AudioMultiPositionComponent::CreateDescriptor());

            m_transformComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioProxyComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioTriggerComponentDescriptor->Reflect(m_serializeContext.get());
            m_audioMultiPosComponentDescriptor->Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_audioMultiPosComponentDescriptor.reset();
            m_audioTriggerComponentDescriptor.reset();
            m_audioMultiPosComponentDescriptor.reset();
            m_audioProxyComponentDescriptor.reset();
            m_transformComponentDescriptor.reset();
            m_serializeContext.reset();
            LeakDetectionFixture::TearDown();
        }

        void CreateDefaultSetup(AZ::Entity& entity)
        {
            entity.CreateComponent<AzFramework::TransformComponent>();
            entity.CreateComponent<LmbrCentral::AudioProxyComponent>();
            entity.CreateComponent<LmbrCentral::AudioTriggerComponent>();
            entity.CreateComponent<LmbrCentral::AudioMultiPositionComponent>();

            entity.Init();
            entity.Activate();
        }
    };

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_ComponentExists)
    {
        AZ::Entity entity;
        CreateDefaultSetup(entity);

        LmbrCentral::AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<LmbrCentral::AudioMultiPositionComponent>();
        EXPECT_TRUE(multiPosComponent != nullptr);
    }

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_AddAndRemoveEntity)
    {
        AZ::Entity entity;
        CreateDefaultSetup(entity);

        LmbrCentral::AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<LmbrCentral::AudioMultiPositionComponent>();
        ASSERT_TRUE(multiPosComponent != nullptr);
        if (multiPosComponent)
        {
            AZ::Entity entity1;
            AZ::Entity entity2;

            EXPECT_EQ(GetNumEntityRefs(multiPosComponent), 0);
            LmbrCentral::AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &LmbrCentral::AudioMultiPositionComponentRequestBus::Events::AddEntity, entity1.GetId());
            EXPECT_EQ(GetNumEntityRefs(multiPosComponent), 1);

            // Remove non-Added
            LmbrCentral::AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &LmbrCentral::AudioMultiPositionComponentRequestBus::Events::RemoveEntity, entity2.GetId());
            EXPECT_EQ(GetNumEntityRefs(multiPosComponent), 1);

            // Remove Added
            LmbrCentral::AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &LmbrCentral::AudioMultiPositionComponentRequestBus::Events::RemoveEntity, entity1.GetId());
            EXPECT_EQ(GetNumEntityRefs(multiPosComponent), 0);
        }
    }

    TEST_F(AudioMultiPositionComponentTests, MultiPositionComponent_EntityActivatedObtainsPosition)
    {
        AZ::Entity entity;
        CreateDefaultSetup(entity);

        LmbrCentral::AudioMultiPositionComponent* multiPosComponent = entity.FindComponent<LmbrCentral::AudioMultiPositionComponent>();
        ASSERT_TRUE(multiPosComponent != nullptr);
        if (multiPosComponent)
        {
            AZ::Entity entity1;
            entity1.Init();
            entity1.Activate();

            LmbrCentral::AudioMultiPositionComponentRequestBus::Event(entity.GetId(), &LmbrCentral::AudioMultiPositionComponentRequestBus::Events::AddEntity, entity1.GetId());

            // Send OnEntityActivated
            AZ::EntityBus::Event(entity1.GetId(), &AZ::EntityBus::Events::OnEntityActivated, entity1.GetId());

            EXPECT_EQ(GetNumEntityRefs(multiPosComponent), 1);
        }
    }

} // namespace UnitTest
