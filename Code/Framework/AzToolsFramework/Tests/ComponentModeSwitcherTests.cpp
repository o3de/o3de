/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentModeTestDoubles.h"
#include "ComponentModeTestFixture.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>



namespace UnitTest
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::ComponentModeFramework;

    using ComponentModeSwitcher = AzToolsFramework::ComponentModeFramework::ComponentModeSwitcher;

    class ComponentModeSwitcherTestFixture : public ComponentModeTestFixture
    {
    };


    /*
    AddComponentModeComponent x
    RemoveCOmponentModeComponent x
    AddingDublicateComponents x
    SelectingAndDeselecting x
    AddingACohd
    AddingEntityToSelectionWithUniqueComponent 
    AddingEntityToSelectionWithDuplicateComponent
    OnEntityComponentAddedAndOnEntityCompositionChanged
    OnEntityComponentRemovedAndOnEntityCompositionChanged
    OnEntityComponentAdded
    OnEnttiyComponentRemoved
    OnEditorModeDeactivated
    OnEditorModeActivated
    ClickingComponentButtonActivatesComponentMode
    */

    TEST_F(ComponentModeSwitcherTestFixture, AddComponentModeComponentAddsComponentToSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);

        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            addComponentsOutcome,
            &EntityCompositionRequests::AddComponentsToEntities,
            entityIds,
            AZ::ComponentTypeList{ AnotherPlaceholderEditorComponent::RTTI_Type() });

        // Then
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);
    }

    TEST_F(ComponentModeSwitcherTestFixture, RemoveComponentModeComponentRemovesComponentFromSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();

        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);

        AzToolsFramework::EntityCompositionRequests::RemoveComponentsOutcome removeComponentsOutcome;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            removeComponentsOutcome,
            &EntityCompositionRequests::RemoveComponents,
            AZ::Entity::ComponentArrayType{ placeholder2 });

        // Then
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddDuplicateComponentModeComponentOnlyAddsOneComponent)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);

        const AzToolsFramework::EntityIdList emptyIds = {};
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, emptyIds);

        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // Then
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);
    }

    TEST_F(ComponentModeSwitcherTestFixture, SelectingAndDeselectingEntitiesAddsAndRemovesComponentsFromSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // Then
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);

        const AzToolsFramework::EntityIdList emptyIds = {};
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, emptyIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 0);

        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddMultipleEntityToSelectionWithSameComponentsKeepCompnentsInSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity2", &entity2);

        // When
        entity->Deactivate();
        entity2->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder3 = entity2->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();
        entity2->Activate();

        // Then
        AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);

        entityIds = { entityId, entityId2 };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddMultipleEntityToSelectionWithUniqueComponentsAddsNoneToSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity2", &entity2);

        // When
        entity->Deactivate();
        entity2->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity2->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();
        entity2->Activate();

        // Then
        AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);

        entityIds = { entityId, entityId2 };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 0);
    }

    TEST_F(ComponentModeSwitcherTestFixture, DeselectOneEntityWithMultipleEntitiesSelectedAddsRemovedComponents)
    {

    }

    TEST_F(ComponentModeSwitcherTestFixture, EnteringComponentModeChangesActiveComponent)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, placeholder1->GetUnderlyingComponentType());

        // Then
        auto activeComponent = componentModeSwitcher->GetActiveComponent()->GetId();
        EXPECT_TRUE(activeComponent == placeholder1->GetId());
    }

    TEST_F(ComponentModeSwitcherTestFixture, LeavingComponentModeChangesActiveComponentToTransformMode)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // When
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, placeholder2->GetUnderlyingComponentType());

        auto activeComponent = componentModeSwitcher->GetActiveComponent()->GetId();
        EXPECT_TRUE(activeComponent == placeholder2->GetId());

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::EndComponentMode);

        // Then
        auto nullComponent = componentModeSwitcher->GetActiveComponent();
        EXPECT_FALSE(nullComponent);
    }

    TEST_F(ComponentModeSwitcherTestFixture, DisableComponentRemovesComponentFromSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);

        EntityCompositionNotificationBus::Broadcast(
            &EntityCompositionNotificationBus::Events::OnEntityComponentDisabled, entity->GetId(), placeholder1->GetId());

        // Then
        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);
    }

    TEST_F(ComponentModeSwitcherTestFixture, EnableComponentRemovesComponentFromSwitcher)
    {
        // Given
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // When
        entity->Deactivate();
        [[maybe_unused]] const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        [[maybe_unused]] AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);

        // Then
        EntityCompositionNotificationBus::Broadcast(
            &EntityCompositionNotificationBus::Events::OnEntityComponentDisabled, entity->GetId(), placeholder1->GetId());

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 1);

        EntityCompositionNotificationBus::Broadcast(
            &EntityCompositionNotificationBus::Events::OnEntityComponentDisabled, entity->GetId(), placeholder1->GetId());

        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{});
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_TRUE(componentModeSwitcher->GetComponentCount() == 2);
    }

} // namespace UnitTest
