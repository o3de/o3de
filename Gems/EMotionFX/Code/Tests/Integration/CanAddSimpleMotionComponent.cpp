/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <Component/EditorComponentAPIBus.h>
#include <Entity/EditorEntityActionComponent.h>

#include <Integration/Editor/Components/EditorActorComponent.h>
#include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#include <Integration/Editor/Components/EditorSimpleMotionComponent.h>
#include <ToolsComponents/EditorPendingCompositionComponent.h>
#include <UI/PropertyEditor/PropertyManagerComponent.h>

#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    using CanAddSimpleMotionComponentFixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AZ::UserSettingsComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        AzToolsFramework::Components::EditorEntityActionComponent,
        AzToolsFramework::Components::EditorComponentAPIComponent,
        EMotionFX::Integration::SystemComponent
    >;

    TEST_F(CanAddSimpleMotionComponentFixture, CanAddSimpleMotionComponent)
    {
        RecordProperty("test_case_id", "C1559180");

        m_app.RegisterComponentDescriptor(Integration::ActorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::AnimGraphComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::SimpleMotionComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::EditorActorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::EditorAnimGraphComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::EditorSimpleMotionComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzToolsFramework::Components::EditorPendingCompositionComponent::CreateDescriptor());

        auto entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(83502341));

        entity->CreateComponent<AzToolsFramework::Components::EditorPendingCompositionComponent>();
        entity->CreateComponent<AzFramework::TransformComponent>();
        entity->CreateComponent<Integration::EditorActorComponent>();
        entity->CreateComponent<Integration::EditorAnimGraphComponent>();

        entity->Init();
        entity->Activate();

        AzToolsFramework::EditorComponentAPIRequests::AddComponentsOutcome componentOutcome;
        AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
            componentOutcome,
            &AzToolsFramework::EditorComponentAPIRequests::AddComponentsOfType,
            entity->GetId(),
            AZ::ComponentTypeList{azrtti_typeid<Integration::EditorSimpleMotionComponent>()}
        );
        ASSERT_TRUE(componentOutcome.IsSuccess()) << componentOutcome.GetError().c_str();

        bool hasComponent = false;
        AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
            hasComponent,
            &AzToolsFramework::EditorComponentAPIRequests::HasComponentOfType,
            entity->GetId(),
            azrtti_typeid<Integration::EditorSimpleMotionComponent>()
        );
        EXPECT_TRUE(hasComponent);

        bool isActive = true;
        AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
            isActive,
            &AzToolsFramework::EditorComponentAPIRequests::IsComponentEnabled,
            componentOutcome.GetValue().at(0)
        );
        EXPECT_FALSE(isActive);
    }
} // namespace EMotionFX
