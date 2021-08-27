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
#include <AzTest/AzTest.h>

#include <ScriptCanvas/AWSScriptBehaviorsComponent.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

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
        m_scriptBehaviorsComponent.reset(m_entity->CreateComponent<AWSScriptBehaviorsComponent>());
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
    AZStd::unique_ptr<AWSScriptBehaviorsComponent> m_scriptBehaviorsComponent;
    AZStd::unique_ptr<AZ::Entity> m_entity;
};

TEST_F(AWSScriptBehaviorsComponentTest, Reflect)
{
    int oldEBusNum = static_cast<int>(m_behaviorContext->m_ebuses.size());
    m_componentDescriptor.reset(AWSScriptBehaviorsComponent::CreateDescriptor());
    m_componentDescriptor->Reflect(m_serializeContext.get());
    m_componentDescriptor->Reflect(m_behaviorContext.get());

    EXPECT_TRUE(m_behaviorContext->m_ebuses.size() - oldEBusNum == 3);
}
