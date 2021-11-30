/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientLocalTicketTracker.h>
#include <AWSGameLiftClientSystemComponent.h>

using namespace AWSGameLift;

class AWSGameLiftClientManagerMock
    : public AWSGameLiftClientManager
{
public:
    AWSGameLiftClientManagerMock() = default;
    ~AWSGameLiftClientManagerMock() = default;

    MOCK_METHOD0(ActivateManager, void());
    MOCK_METHOD0(DeactivateManager, void());
};

class AWSGameLiftClientLocalTicketTrackerMock
    : public AWSGameLiftClientLocalTicketTracker
{
public:
    AWSGameLiftClientLocalTicketTrackerMock() = default;
    ~AWSGameLiftClientLocalTicketTrackerMock() = default;

    MOCK_METHOD0(ActivateTracker, void());
    MOCK_METHOD0(DeactivateTracker, void());
};

class TestAWSGameLiftClientSystemComponent
    : public AWSGameLiftClientSystemComponent
{
public:
    TestAWSGameLiftClientSystemComponent()
    {
        m_gameliftClientManagerMockPtr = nullptr;
        m_gameliftClientTicketTrackerMockPtr = nullptr;
    }
    ~TestAWSGameLiftClientSystemComponent()
    {
        m_gameliftClientManagerMockPtr = nullptr;
        m_gameliftClientTicketTrackerMockPtr = nullptr;
    }

    void SetUpMockManager()
    {
        AZStd::unique_ptr<AWSGameLiftClientManagerMock> gameliftClientManagerMock =
            AZStd::make_unique<AWSGameLiftClientManagerMock>();
        m_gameliftClientManagerMockPtr = gameliftClientManagerMock.get();
        SetGameLiftClientManager(AZStd::move(gameliftClientManagerMock));

        AZStd::unique_ptr<AWSGameLiftClientLocalTicketTrackerMock> gameliftClientTicketTrackerMock =
            AZStd::make_unique<AWSGameLiftClientLocalTicketTrackerMock>();
        m_gameliftClientTicketTrackerMockPtr = gameliftClientTicketTrackerMock.get();
        SetGameLiftClientTicketTracker(AZStd::move(gameliftClientTicketTrackerMock));
    }

    AWSGameLiftClientManagerMock* m_gameliftClientManagerMockPtr;
    AWSGameLiftClientLocalTicketTrackerMock* m_gameliftClientTicketTrackerMockPtr;
};

class AWSCoreSystemComponentMock
    : public AZ::Component
{
public:
    AZ_COMPONENT(AWSCoreSystemComponentMock, "{52DB1342-30C6-412F-B7CC-B23F8B0629EA}");

    static void Reflect(AZ::ReflectContext* context)
    {
        AZ_UNUSED(context);
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

    AWSCoreSystemComponentMock() = default;
    ~AWSCoreSystemComponentMock() = default;

    void Init() override {}
    void Activate() override {}
    void Deactivate() override {}
};

class AWSGameLiftClientSystemComponentTest
    : public AWSGameLiftClientFixture
{
protected:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_coreComponentDescriptor;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_gameliftClientComponentDescriptor;

    void SetUp() override
    {
        AWSGameLiftClientFixture::SetUp();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
        m_coreComponentDescriptor.reset(AWSCoreSystemComponentMock::CreateDescriptor());
        m_gameliftClientComponentDescriptor.reset(TestAWSGameLiftClientSystemComponent::CreateDescriptor());
        m_gameliftClientComponentDescriptor->Reflect(m_serializeContext.get());
        m_gameliftClientComponentDescriptor->Reflect(m_behaviorContext.get());

        m_entity = aznew AZ::Entity();
        m_coreSystemComponent = AZStd::make_unique<AWSCoreSystemComponentMock>();
        m_entity->AddComponent(m_coreSystemComponent.get());
        m_gameliftClientSystemComponent = AZStd::make_unique<TestAWSGameLiftClientSystemComponent>();
        m_gameliftClientSystemComponent->SetUpMockManager();
        m_entity->AddComponent(m_gameliftClientSystemComponent.get());
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_gameliftClientSystemComponent.get());
        m_gameliftClientSystemComponent.reset();
        m_entity->RemoveComponent(m_coreSystemComponent.get());
        m_coreSystemComponent.reset();
        delete m_entity;
        m_entity = nullptr;

        m_gameliftClientComponentDescriptor.reset();
        m_coreComponentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();

        AWSGameLiftClientFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSCoreSystemComponentMock> m_coreSystemComponent;
    AZStd::unique_ptr<TestAWSGameLiftClientSystemComponent> m_gameliftClientSystemComponent;
    AZ::Entity* m_entity;
};

TEST_F(AWSGameLiftClientSystemComponentTest, ActivateDeactivate_Call_GameLiftClientManagerGetsInvoked)
{
    m_entity->Init();
    EXPECT_CALL(*(m_gameliftClientSystemComponent->m_gameliftClientManagerMockPtr), ActivateManager()).Times(1);
    EXPECT_CALL(*(m_gameliftClientSystemComponent->m_gameliftClientTicketTrackerMockPtr), ActivateTracker()).Times(1);
    m_entity->Activate();

    EXPECT_CALL(*(m_gameliftClientSystemComponent->m_gameliftClientManagerMockPtr), DeactivateManager()).Times(1);
    EXPECT_CALL(*(m_gameliftClientSystemComponent->m_gameliftClientTicketTrackerMockPtr), DeactivateTracker()).Times(1);
    m_entity->Deactivate();
}
