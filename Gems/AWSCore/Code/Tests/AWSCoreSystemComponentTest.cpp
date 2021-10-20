/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobCancelGroup.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzTest/AzTest.h>

#include <AWSCoreSystemComponent.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <Credential/AWSCredentialManager.h>
#include <Framework/AWSApiJob.h>
#include <ResourceMapping/AWSResourceMappingManager.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSCoreNotificationsBusMock
    : protected AWSCoreNotificationsBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(AWSCoreNotificationsBusMock, AZ::SystemAllocator, 0);

    AWSCoreNotificationsBusMock()
        : m_sdkInitialized(0)
        , m_sdkShutdownStarted(0)
    {
        AWSCoreNotificationsBus::Handler::BusConnect();
    }

    ~AWSCoreNotificationsBusMock() override
    {
        AWSCoreNotificationsBus::Handler::BusDisconnect();
    }

    //////////////////////////////////////////////////////////////////////////
    // handles AWSCoreNotificationsBus
    void OnSDKInitialized() override { m_sdkInitialized++; }
    void OnSDKShutdownStarted() override {  m_sdkShutdownStarted++; }

    int m_sdkInitialized;
    int m_sdkShutdownStarted;
};

class AWSCoreSystemComponentTest
    : public AWSCoreFixture
{
protected:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;

    void SetUp() override
    {
        AWSCoreFixture::SetUp();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

        m_componentDescriptor.reset(AWSCoreSystemComponent::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_componentDescriptor->Reflect(m_behaviorContext.get());

        m_settingsRegistry->SetContext(m_serializeContext.get());

        m_entity = aznew AZ::Entity();
        m_coreSystemsComponent.reset(m_entity->CreateComponent<AWSCoreSystemComponent>());
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_coreSystemsComponent.get());
        m_coreSystemsComponent.reset();
        delete m_entity;
        m_entity = nullptr;

        m_coreSystemsComponent.reset();
        m_componentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();

        AWSCoreFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSCoreSystemComponent> m_coreSystemsComponent;
    AZ::Entity* m_entity;
    AWSCoreNotificationsBusMock m_notifications;
};

TEST_F(AWSCoreSystemComponentTest, ComponentActivateTest)
{
    EXPECT_FALSE(m_coreSystemsComponent->IsAWSApiInitialized());

    // activate component
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_entity->Init();
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // expect the above have thrown an AZ_Error
    m_entity->Activate();

    EXPECT_EQ(m_notifications.m_sdkInitialized, 1);
    EXPECT_TRUE(m_coreSystemsComponent->IsAWSApiInitialized());

    // deactivate component
    m_entity->Deactivate();
    EXPECT_EQ(m_notifications.m_sdkShutdownStarted, 1);

    EXPECT_FALSE(m_coreSystemsComponent->IsAWSApiInitialized());
}

TEST_F(AWSCoreSystemComponentTest, GetDefaultJobContext_Call_JobContextIsNotNullptr)
{
    auto actualContext = m_coreSystemsComponent->GetDefaultJobContext();
    EXPECT_TRUE(actualContext);
}

TEST_F(AWSCoreSystemComponentTest, GetDefaultConfig_Call_GetConfigWithExpectedValue)
{
    auto actualDefaultConfig = m_coreSystemsComponent->GetDefaultConfig();
    EXPECT_TRUE(actualDefaultConfig->userAgent == "/O3DE_AwsApiJob");
    EXPECT_TRUE(actualDefaultConfig->requestTimeoutMs == 30000);
    EXPECT_TRUE(actualDefaultConfig->connectTimeoutMs == 30000);

    AWSCoreNotificationsBus::Broadcast(&AWSCoreNotifications::OnSDKShutdownStarted);
}
