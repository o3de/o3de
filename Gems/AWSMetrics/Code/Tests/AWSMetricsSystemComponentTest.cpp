/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSCoreBus.h>
#include <AWSMetricsSystemComponent.h>
#include <AWSMetricsGemMock.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/Component/Entity.h>

namespace AWSMetrics
{
    class AWSMetricsSystemComponentMock
        : public AWSMetricsSystemComponent
    {
    public:
        void InitMock()
        {
            AWSMetricsSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSMetricsSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSMetricsSystemComponent::Deactivate();
        }

        AWSMetricsSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSMetricsSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSMetricsSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSMetricsSystemComponentMock::DeactivateMock));
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
    };

    class AWSCoreSystemComponentMock
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(AWSCoreSystemComponentMock, "{D1D84E43-66FA-470B-9762-AE253EF46F92}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<AWSCoreSystemComponentMock, AZ::Component>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AWSCoreSystemComponentMock>("AWSCoreMock", "Adds core support for working with AWS")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AWSCoreService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            AZ_UNUSED(incompatible);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void ActivateMock()
        {
            AWSCore::AWSCoreNotificationsBus::Broadcast(&AWSCore::AWSCoreNotifications::OnSDKInitialized);
        }

        AWSCoreSystemComponentMock()
        {
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSCoreSystemComponentMock::ActivateMock));
        }

        ~AWSCoreSystemComponentMock() = default;

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
    };

    class AWSMetricsSystemComponentTest
        : public AWSMetricsGemAllocatorFixture
    {
    protected:
        void SetUp() override
        {
            AWSMetricsGemAllocatorFixture::SetUp();

            m_awsCoreComponentDescriptor.reset(AWSCoreSystemComponentMock::CreateDescriptor());
            m_componentDescriptor.reset(AWSMetricsSystemComponentMock::CreateDescriptor());

            m_awsCoreComponentDescriptor->Reflect(m_serializeContext.get());
            m_componentDescriptor->Reflect(m_serializeContext.get());

            m_entity = aznew AZ::Entity();
            m_awsCoreSystemsComponent = aznew testing::NiceMock<AWSCoreSystemComponentMock>();
            m_AWSMetricsSystemsComponent = aznew testing::NiceMock<AWSMetricsSystemComponentMock>();
            m_entity->AddComponent(m_awsCoreSystemsComponent);
            m_entity->AddComponent(m_AWSMetricsSystemsComponent);
        }

        void TearDown() override
        {
            m_entity->RemoveComponent(m_AWSMetricsSystemsComponent);
            m_entity->RemoveComponent(m_awsCoreSystemsComponent);
            delete m_AWSMetricsSystemsComponent;
            delete m_awsCoreSystemsComponent;
            delete m_entity;

            m_componentDescriptor.reset();
            m_awsCoreComponentDescriptor.reset();

            AWSMetricsGemAllocatorFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_awsCoreComponentDescriptor;

    public:
        testing::NiceMock<AWSMetricsSystemComponentMock>* m_AWSMetricsSystemsComponent = nullptr;
        testing::NiceMock<AWSCoreSystemComponentMock>* m_awsCoreSystemsComponent = nullptr;
        AZ::Entity* m_entity = nullptr;
    };

    TEST_F(AWSMetricsSystemComponentTest, ActivateComponent_NewEntity_Success)
    {
        testing::Sequence s1, s2;

        EXPECT_CALL(*m_awsCoreSystemsComponent, Init()).Times(1).InSequence(s1);
        EXPECT_CALL(*m_AWSMetricsSystemsComponent, Init()).Times(1).InSequence(s1);
        EXPECT_CALL(*m_awsCoreSystemsComponent, Activate()).Times(1).InSequence(s1);
        EXPECT_CALL(*m_AWSMetricsSystemsComponent, Activate()).Times(1).InSequence(s1);

        EXPECT_CALL(*m_AWSMetricsSystemsComponent, Deactivate()).Times(1).InSequence(s2);
        EXPECT_CALL(*m_awsCoreSystemsComponent, Deactivate()).Times(1).InSequence(s2);

        // initialize component
        m_entity->Init();

        // activate component
        m_entity->Activate();

        // deactivate component
        m_entity->Deactivate();
    }
}
