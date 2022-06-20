/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftServerFixture.h>
#include <AWSGameLiftServerMocks.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class AWSGameLiftServerSystemComponentTest
        : public AWSGameLiftServerFixture
    {
    public:
        void SetUp() override
        {
            AWSGameLiftServerFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_serializeContext->CreateEditContext();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_componentDescriptor.reset(AWSGameLift::AWSGameLiftServerSystemComponent::CreateDescriptor());
            m_componentDescriptor->Reflect(m_serializeContext.get());
            m_componentDescriptor->Reflect(m_behaviorContext.get());

            m_entity = aznew AZ::Entity();

            m_AWSGameLiftServerSystemsComponent = aznew NiceMock<AWSGameLiftServerSystemComponentMock>();
            m_entity->AddComponent(m_AWSGameLiftServerSystemsComponent);
        }

        void TearDown() override
        {
            m_entity->RemoveComponent(m_AWSGameLiftServerSystemsComponent);
            delete m_AWSGameLiftServerSystemsComponent;
            delete m_entity;

            m_componentDescriptor.reset();
            m_behaviorContext.reset();
            m_serializeContext.reset();

            AWSGameLiftServerFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;

        AZ::Entity* m_entity;      
        NiceMock<AWSGameLiftServerSystemComponentMock>* m_AWSGameLiftServerSystemsComponent;
    };

    TEST_F(AWSGameLiftServerSystemComponentTest, ActivateDeactivateComponent_ExecuteInOrder_Success)
    {
        testing::Sequence s1, s2;

        EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Init()).Times(1).InSequence(s1);
        EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Activate()).Times(1).InSequence(s1);

        EXPECT_CALL(*m_AWSGameLiftServerSystemsComponent, Deactivate()).Times(1).InSequence(s2);

        // activate component
        m_entity->Init();
        m_entity->Activate();

        // deactivate component
        m_entity->Deactivate();
    }

} // namespace UnitTest
