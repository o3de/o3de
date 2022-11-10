/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/ComponentModeTestDoubles.h>
#include <Tests/ComponentModeTestFixture.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <AzToolsFramework/ViewportUi/ViewportUiSwitcher.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

namespace UnitTest
{
    using AzToolsFramework::ComponentModeFramework::AnotherPlaceholderEditorComponent;
    using AzToolsFramework::ComponentModeFramework::PlaceholderEditorComponent;
    using ComponentModeSwitcher = AzToolsFramework::ComponentModeFramework::ComponentModeSwitcher;
    using ComponentModeSwitcherTestFixture = ComponentModeTestFixture;

    TEST_F(ComponentModeSwitcherTestFixture, AddingComponentsToEntityAddsComponentsToSwitcher)
    {
        // Given the setup of one entity with one component
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        // When the entity is selected, expect the switcher to have one component
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());

        // Then when another component is added to the entity, expect the switcher to have two components
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addComponentsOutcome;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            addComponentsOutcome,
            &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities,
            entityIds,
            AZ::ComponentTypeList{ AnotherPlaceholderEditorComponent::RTTI_Type() });

        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, RemovingComponentsFromEntityRemovesComponentsFromSwitcher)
    {
        // Given the set up of one entity selected with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // When the user selects the entity, two components show up in the switcher
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        // Then when the user removes a component, one component remains
        AzToolsFramework::EntityCompositionRequests::RemoveComponentsOutcome removeComponentsOutcome;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            removeComponentsOutcome,
            &AzToolsFramework::EntityCompositionRequests::RemoveComponents,
            AZ::Entity::ComponentArrayType{ placeholder2 });

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, InstantaneousChangeOfEntitySelectionUpdatesSwitcherCorrectly)
    {
        // Given an entity with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity", &entity2);

        entity->Deactivate();
        entity2->Deactivate();

        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity2->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity->Activate();
        entity2->Activate();

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId });

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, AzToolsFramework::EntityIdList{ entityId2 });

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddingDuplicateComponentsDoesNotAddComponentsToSwitcher)
    {
        // Given an entity with one component
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        // When an entity is selected, there is one component added to the switcher
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());

        // Then if the user adds an identical component, there is still one component on the switcher
        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->Activate();

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, SelectingAndDeselectingEntitiesAddsAndRemovesComponentsFromSwitcher)
    {
        // Given an entity with multiple components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // When the entity is selected, there are two components on the switcher
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        // Then when the entity is deselected
        const AzToolsFramework::EntityIdList emptyIds = {};
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, emptyIds);
        EXPECT_EQ(0, componentModeSwitcher->GetComponentCount());

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddIngMultipleEntitiesToSelectionWithSameComponentsKeepComponentsInSwitcher)
    {
        // Given two entities with different components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity2", &entity2);

        entity->Deactivate();
        entity2->Deactivate();

        entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity2->CreateComponent<PlaceholderEditorComponent>();

        entity->Activate();
        entity2->Activate();

        // When one entity is selected all associated components show up in the switcher
        AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        // Then if both entities are selected, only components that are shared between both entities show up
        entityIds = { entityId, entityId2 };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, AddingMultipleEntityToSelectionWithUniqueComponentsRemovesUniqueFromSwitcher)
    {
        // Given two entities with multiple components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity2", &entity2);

        entity->Deactivate();
        entity2->Deactivate();

        entity->CreateComponent<PlaceholderEditorComponent>();
        entity2->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity->Activate();
        entity2->Activate();

        // When one entity is selected the component shows up like normal
        AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());

        // When both entities are selected, if there are no common components, the switcher is empty
        entityIds = { entityId, entityId2 };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        EXPECT_EQ(0, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, DeselectingOneEntityWithMultipleEntitiesSelectedAddsRemovedComponents)
    {
        // Given two entities with different components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::Entity* entity2 = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);
        AZ::EntityId entityId2 = CreateDefaultEditorEntity("ComponentModeEntity2", &entity2);

        entity->Deactivate();
        entity2->Deactivate();

        entity->CreateComponent<PlaceholderEditorComponent>();
        entity2->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity->Activate();
        entity2->Activate();

        // When the both entities are selected, nothing shows up in the switcher as there are no common components
        AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());

        entityIds = { entityId, entityId2 };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(0, componentModeSwitcher->GetComponentCount());

        // When the second entity is removed from the selection, the switcher now has the component from the single entity selected
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::DeleteEntityById, entityId2);

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, EnteringComponentModeChangesActiveComponent)
    {
        // Given an entity with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // When component mode is activated for a component in any way (through the switcher or the entity tab)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::AddSelectedComponentModesOfType,
            placeholder1->GetUnderlyingComponentType());

        // Then the switchers active button is the component that component mode is active for
        auto activeComponent = componentModeSwitcher->GetActiveComponent()->GetId();

        EXPECT_EQ(activeComponent, placeholder1->GetId());
    }

    TEST_F(ComponentModeSwitcherTestFixture, LeavingComponentModeChangesActiveComponentToTransformMode)
    {
        // Given an entity with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);

        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        entity->CreateComponent<PlaceholderEditorComponent>();
        const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // When component mode is de-activated for a component in any way (through the switcher or the entity tab)
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::AddSelectedComponentModesOfType,
            placeholder2->GetUnderlyingComponentType());

        auto activeComponent = componentModeSwitcher->GetActiveComponent()->GetId();
        EXPECT_EQ(activeComponent, placeholder2->GetId());

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::EndComponentMode);

        // Then the active switcher button should be the transform component (indicated by null)
        auto nullComponent = componentModeSwitcher->GetActiveComponent();

        EXPECT_EQ(nullComponent, nullptr);
    }

    TEST_F(ComponentModeSwitcherTestFixture, DisablingComponentRemovesComponentFromSwitcher)
    {
        // Given an entity with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();
        AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // When the entity is selected there should be two components in the switcher
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        // Then if one component is disabled, there should only be one component on the switcher
        AzToolsFramework::EntityCompositionRequestBus::Broadcast(
            &AzToolsFramework::EntityCompositionRequests::DisableComponents, AZStd::vector<AZ::Component*>{ placeholder1 });

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());
    }

    TEST_F(ComponentModeSwitcherTestFixture, EnablingComponentAddsComponentToSwitcher)
    {
        // Given an entity with two components
        AZStd::shared_ptr<ComponentModeSwitcher> componentModeSwitcher = AZStd::make_shared<ComponentModeSwitcher>();

        AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::OverrideComponentModeSwitcher,
            componentModeSwitcher);
            
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        // connect to EditorDisabledCompositionRequestBus with entityId
        Connect(entityId);

        entity->Deactivate();
        AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        entity->Activate();

        // When the component is disabled it doesn't show up in the switcher
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);
        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(
            &AzToolsFramework::EntityCompositionRequests::DisableComponents, AZStd::vector<AZ::Component*>{ placeholder1 });

        AzToolsFramework::EditorDisabledCompositionRequestBus::Event(
            entityId, &AzToolsFramework::EditorDisabledCompositionRequests::AddDisabledComponent, placeholder1);

        EXPECT_EQ(1, componentModeSwitcher->GetComponentCount());

        AddDisabledComponentToBus(placeholder1);

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(
            &AzToolsFramework::EntityCompositionRequests::EnableComponents, AZStd::vector<AZ::Component*>{ placeholder1 });

        EXPECT_EQ(2, componentModeSwitcher->GetComponentCount());

        Disconnect();
    }
} // namespace UnitTest
