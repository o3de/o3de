/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Attribution/AWSCoreAttributionSystemComponent.h>
#include <TestFramework/AWSCoreFixture.h>


using namespace AWSCore;

class AWSAttributionSystemComponentTest
    : public AWSCoreFixture
{
    void SetUp() override
    {
        AWSCoreFixture::SetUp();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
        m_componentDescriptor.reset(AWSAttributionSystemComponent::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_componentDescriptor->Reflect(m_behaviorContext.get());

        m_entity = aznew AZ::Entity();
        m_attributionSystemsComponent.reset(m_entity->CreateComponent<AWSAttributionSystemComponent>());
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_entity->Init();
        AZ_TEST_STOP_TRACE_SUPPRESSION(0); 
        m_entity->Activate();
    }

    void TearDown() override
    {
        m_entity->Deactivate();
        m_entity->RemoveComponent(m_attributionSystemsComponent.get());
        m_attributionSystemsComponent.reset();
        delete m_entity;
        m_entity = nullptr;

        m_attributionSystemsComponent.reset();
        m_componentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();

        AWSCoreFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSAttributionSystemComponent> m_attributionSystemsComponent;
    AZ::Entity* m_entity;

private:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
};

TEST_F(AWSAttributionSystemComponentTest, EmptyTest)
{

}
