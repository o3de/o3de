/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AWSCoreEditorSystemComponent.h>

#include <Editor/UI/AWSCoreEditorUIFixture.h>
#include <TestFramework/AWSCoreFixture.h>

#include <QMainWindow>
#include <QMenuBar>

using namespace AWSCore;

class AWSCoreEditorSystemComponentTest
    : public AWSCoreFixture
    , public AWSCoreEditorUIFixture
{
    void SetUp() override
    {
        AWSCoreEditorUIFixture::SetUp();
        AWSCoreFixture::SetUp();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
        m_componentDescriptor.reset(AWSCoreEditorSystemComponent::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_componentDescriptor->Reflect(m_behaviorContext.get());

        m_entity = aznew AZ::Entity();
        m_coreEditorSystemsComponent.reset(m_entity->CreateComponent<AWSCoreEditorSystemComponent>());
        m_entity->Init();
        m_entity->Activate();
    }

    void TearDown() override
    {
        m_entity->Deactivate();
        m_entity->RemoveComponent(m_coreEditorSystemsComponent.get());
        m_coreEditorSystemsComponent.reset();
        delete m_entity;
        m_entity = nullptr;

        m_coreEditorSystemsComponent.reset();
        m_componentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();

        AWSCoreFixture::TearDown();
        AWSCoreEditorUIFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSCoreEditorSystemComponent> m_coreEditorSystemsComponent;
    AZ::Entity* m_entity;

private:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
};
