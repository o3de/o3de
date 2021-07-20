/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <ScriptCanvas/AWSScriptBehaviorsComponent.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

class AWSScriptBehaviorsComponentMock
    : public AWSScriptBehaviorsComponent
{
public:
    AZ_COMPONENT(AWSScriptBehaviorsComponentMock, "{78579706-E1B2-4788-A34D-A58D3F273FF9}");

    int GetBehaviorsNum()
    {
        return m_behaviors.size();
    }
};

class AWSScriptBehaviorsComponentTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:
    void SetUp() override
    {
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

        m_entity = AZStd::make_unique<AZ::Entity>();
        m_scriptBehaviorsComponent.reset(m_entity->CreateComponent<AWSScriptBehaviorsComponentMock>());
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_scriptBehaviorsComponent.get());
        m_scriptBehaviorsComponent.reset();
        m_entity.reset();

        m_componentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();
    }

protected:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
    AZStd::unique_ptr<AWSScriptBehaviorsComponentMock> m_scriptBehaviorsComponent;
    AZStd::unique_ptr<AZ::Entity> m_entity;
};

TEST_F(AWSScriptBehaviorsComponentTest, InitActivateDeactivate_Call_GetExpectedNumOfAddedBehaviors)
{
    m_componentDescriptor.reset(AWSScriptBehaviorsComponentMock::CreateDescriptor());
    m_componentDescriptor->Reflect(m_serializeContext.get());
    m_componentDescriptor->Reflect(m_behaviorContext.get());
    EXPECT_TRUE(AWSScriptBehaviorsComponentMock::AddedBehaviours());
    EXPECT_TRUE(m_scriptBehaviorsComponent->GetBehaviorsNum() == 3);

    m_entity->Init();
    m_entity->Activate();
    m_entity->Deactivate();
    EXPECT_TRUE(m_scriptBehaviorsComponent->GetBehaviorsNum() == 0);
}
