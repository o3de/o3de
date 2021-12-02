/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AWSClientAuthSystemComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AWSClientAuthGemMock.h>

namespace AWSClientAuthUnitTest
{
    class AWSClientAuthSystemComponentMock
        : public AWSClientAuth::AWSClientAuthSystemComponent
    {
    public:
        using AWSClientAuth::AWSClientAuthSystemComponent::GetCognitoIDPClient;
        using AWSClientAuth::AWSClientAuthSystemComponent::GetCognitoIdentityClient;

        void InitMock()
        {
            AWSClientAuth::AWSClientAuthSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSClientAuth::AWSClientAuthSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSClientAuth::AWSClientAuthSystemComponent::Deactivate();
        }

        AWSClientAuthSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSClientAuthSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSClientAuthSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSClientAuthSystemComponentMock::DeactivateMock));
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());

        AZStd::vector<AWSClientAuth::ProviderNameEnum> m_enabledProviderNames;        
    };

    class AWSCoreSystemComponentMock
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(AWSCoreSystemComponentMock, "{5F48030D-EB59-4820-BC65-69EC7CC6C119}");
 
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
}

class AWSClientAuthSystemComponentTest
    : public AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture
{
public:
    AWSClientAuthSystemComponentTest()
        : AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture(false)
    {

    }
protected:
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_awsCoreComponentDescriptor;

    void SetUp() override
    {
        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::SetUp();

        m_componentDescriptor.reset(AWSClientAuth::AWSClientAuthSystemComponent::CreateDescriptor());
        m_awsCoreComponentDescriptor.reset(AWSClientAuthUnitTest::AWSCoreSystemComponentMock::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_awsCoreComponentDescriptor->Reflect(m_serializeContext.get());

        m_entity = aznew AZ::Entity();

        m_awsClientAuthSystemsComponent = aznew testing::NiceMock<AWSClientAuthUnitTest::AWSClientAuthSystemComponentMock>();
        m_awsCoreSystemsComponent = aznew testing::NiceMock<AWSClientAuthUnitTest::AWSCoreSystemComponentMock>();
        m_entity->AddComponent(m_awsCoreSystemsComponent);
        m_entity->AddComponent(m_awsClientAuthSystemsComponent);
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_awsClientAuthSystemsComponent);
        m_entity->RemoveComponent(m_awsCoreSystemsComponent);
        delete m_awsCoreSystemsComponent;
        delete m_awsClientAuthSystemsComponent;
        delete m_entity;

        m_componentDescriptor.reset();
        m_awsCoreComponentDescriptor.reset();

        AWSClientAuthUnitTest::AWSClientAuthGemAllocatorFixture::TearDown();
    }

public:
    testing::NiceMock<AWSClientAuthUnitTest::AWSClientAuthSystemComponentMock> *m_awsClientAuthSystemsComponent;
    testing::NiceMock<AWSClientAuthUnitTest::AWSCoreSystemComponentMock> *m_awsCoreSystemsComponent;
    testing::NiceMock<AWSClientAuthUnitTest::AWSResourceMappingRequestBusMock> m_awsResourceMappingRequestBusMock;
    testing::NiceMock<AWSClientAuthUnitTest::AWSCoreRequestBusMock> m_awsCoreRequestBusMock;
    AZ::Entity* m_entity = nullptr;
};



TEST_F(AWSClientAuthSystemComponentTest, ActivateDeactivate_Success)
{
    m_awsClientAuthSystemsComponent->m_enabledProviderNames.push_back(AWSClientAuth::ProviderNameEnum::LoginWithAmazon);
    m_awsClientAuthSystemsComponent->m_enabledProviderNames.push_back(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    testing::Sequence s1, s2;

    EXPECT_CALL(*m_awsCoreSystemsComponent, Init()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_awsClientAuthSystemsComponent, Init()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_awsCoreSystemsComponent, Activate()).Times(1).InSequence(s1);
    EXPECT_CALL(m_awsCoreRequestBusMock, GetDefaultConfig()).Times(1).InSequence(s1);
    EXPECT_CALL(m_awsResourceMappingRequestBusMock, GetDefaultRegion()).Times(1).InSequence(s1);
    EXPECT_CALL(*m_awsClientAuthSystemsComponent, Activate()).Times(1).InSequence(s1);

    EXPECT_CALL(*m_awsClientAuthSystemsComponent, Deactivate()).Times(1).InSequence(s2);
    EXPECT_CALL(*m_awsCoreSystemsComponent, Deactivate()).Times(1).InSequence(s2);

    // activate component
    m_entity->Init();
    m_entity->Activate();

    // deactivate component
    m_entity->Deactivate();
}

TEST_F(AWSClientAuthSystemComponentTest, GetCognitoClients_Success)
{
    m_awsClientAuthSystemsComponent->m_enabledProviderNames.push_back(AWSClientAuth::ProviderNameEnum::LoginWithAmazon);
    m_awsClientAuthSystemsComponent->m_enabledProviderNames.push_back(AWSClientAuth::ProviderNameEnum::AWSCognitoIDP);

    // activate component
    m_entity->Init();
    m_entity->Activate();

    EXPECT_TRUE(AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIdentityClient() != nullptr);
    EXPECT_TRUE(AZ::Interface<IAWSClientAuthRequests>::Get()->GetCognitoIDPClient() != nullptr);

    // deactivate component
    m_entity->Deactivate();
}
