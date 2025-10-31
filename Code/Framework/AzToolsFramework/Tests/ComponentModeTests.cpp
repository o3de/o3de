/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeTestDoubles.h"
#include "ComponentModeTestFixture.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <QApplication>

namespace UnitTest
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::ComponentModeFramework;

    TEST_F(ComponentModeTestFixture, BeginEndComponentMode)
    {
        using ::testing::Eq;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        QWidget rootWidget;
        ActionOverrideRequestBus::Event(GetEntityContextId(), &ActionOverrideRequests::SetupActionOverrideHandler, &rootWidget);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::BeginComponentMode, AZStd::vector<EntityAndComponentModeBuilders>{});

        bool inComponentMode = false;
        ComponentModeSystemRequestBus::BroadcastResult(inComponentMode, &ComponentModeSystemRequests::InComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(inComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        ComponentModeSystemRequestBus::Broadcast(&ComponentModeSystemRequests::EndComponentMode);

        ComponentModeSystemRequestBus::BroadcastResult(inComponentMode, &ComponentModeSystemRequests::InComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(inComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ActionOverrideRequestBus::Event(GetEntityContextId(), &ActionOverrideRequests::TeardownActionOverrideHandler);
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithSameComponentModeBothBegin)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<PlaceholderEditorComponent>::Uuid());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        bool firstComponentModeInstantiated = false;
        ComponentModeSystemRequestBus::BroadcastResult(
            firstComponentModeInstantiated, &ComponentModeSystemRequests::ComponentModeInstantiated,
            AZ::EntityComponentIdPair(entityId, placeholder1->GetId()));

        bool secondComponentModeInstantiated = false;
        ComponentModeSystemRequestBus::BroadcastResult(
            secondComponentModeInstantiated, &ComponentModeSystemRequests::ComponentModeInstantiated,
            AZ::EntityComponentIdPair(entityId, placeholder2->GetId()));

        EXPECT_TRUE(firstComponentModeInstantiated && secondComponentModeInstantiated);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, OneComponentModeBeginsWithTwoComponentsOnSingleEntityEachWithDifferentComponentModes)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<PlaceholderEditorComponent>::Uuid());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        bool firstComponentModeInstantiated = false;
        ComponentModeSystemRequestBus::BroadcastResult(
            firstComponentModeInstantiated, &ComponentModeSystemRequests::ComponentModeInstantiated,
            AZ::EntityComponentIdPair(entityId, placeholder1->GetId()));

        bool secondComponentModeInstantiated = true;
        ComponentModeSystemRequestBus::BroadcastResult(
            secondComponentModeInstantiated, &ComponentModeSystemRequests::ComponentModeInstantiated,
            AZ::EntityComponentIdPair(entityId, placeholder2->GetId()));

        EXPECT_TRUE(firstComponentModeInstantiated && !secondComponentModeInstantiated);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithSameComponentModeDoNotCycle)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        AZ_UNUSED(placeholder1);
        const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();
        AZ_UNUSED(placeholder2);

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<PlaceholderEditorComponent>::Uuid());

        bool nextModeCycled = true;
        ComponentModeSystemRequestBus::BroadcastResult(nextModeCycled, &ComponentModeSystemRequests::SelectNextActiveComponentMode);

        bool previousModeCycled = true;
        ComponentModeSystemRequestBus::BroadcastResult(previousModeCycled, &ComponentModeSystemRequests::SelectPreviousActiveComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(!nextModeCycled && !previousModeCycled);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithSameComponentModeHasOnlyOneType)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        AZ_UNUSED(placeholder1);
        const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();
        AZ_UNUSED(placeholder2);

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<PlaceholderEditorComponent>::Uuid());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        bool multipleComponentModeTypes = true;
        ComponentModeSystemRequestBus::BroadcastResult(multipleComponentModeTypes, &ComponentModeSystemRequests::HasMultipleComponentTypes);

        EXPECT_FALSE(multipleComponentModeTypes);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithDifferentComponentModeHasOnlyOneType)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        AZ_UNUSED(placeholder1);
        const AZ::Component* placeholder2 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        AZ_UNUSED(placeholder2);

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<AnotherPlaceholderEditorComponent>::Uuid());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        bool multipleComponentModeTypes = true;
        ComponentModeSystemRequestBus::BroadcastResult(multipleComponentModeTypes, &ComponentModeSystemRequests::HasMultipleComponentTypes);

        EXPECT_FALSE(multipleComponentModeTypes);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithDependentComponentModesHasTwoTypes)
    {
        using testing::Eq;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // setup default editor interaction model
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<AnotherPlaceholderEditorComponent>();
        AZ_UNUSED(placeholder1);
        // DependentPlaceholderEditorComponent has a Component Mode dependent on AnotherPlaceholderEditorComponent
        const AZ::Component* placeholder2 = entity->CreateComponent<DependentPlaceholderEditorComponent>();
        AZ_UNUSED(placeholder2);

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        const AzToolsFramework::EntityIdList entityIds = { entityId };
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequests::AddSelectedComponentModesOfType, AZ::AzTypeInfo<DependentPlaceholderEditorComponent>::Uuid());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        bool multipleComponentModeTypes = false;
        ComponentModeSystemRequestBus::BroadcastResult(multipleComponentModeTypes, &ComponentModeSystemRequests::HasMultipleComponentTypes);

        bool secondComponentModeInstantiated = false;
        ComponentModeSystemRequestBus::BroadcastResult(
            secondComponentModeInstantiated, &ComponentModeSystemRequests::ComponentModeInstantiated,
            AZ::EntityComponentIdPair(entityId, placeholder2->GetId()));

        AZ::Uuid activeComponentType = AZ::Uuid::CreateNull();
        ComponentModeSystemRequestBus::BroadcastResult(activeComponentType, &ComponentModeSystemRequests::ActiveComponentMode);

        EXPECT_TRUE(multipleComponentModeTypes);
        EXPECT_TRUE(secondComponentModeInstantiated);
        EXPECT_THAT(activeComponentType, Eq(AZ::AzTypeInfo<DependentPlaceholderEditorComponent>::Uuid()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, TwoComponentsOnSingleEntityWithSameComponentModeBothTriggerSameAction)
    {
        using testing::Eq;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // add two placeholder Components (each with their own Component Mode)
        const AZ::Component* placeholder1 = entity->CreateComponent<PlaceholderEditorComponent>();
        const AZ::Component* placeholder2 = entity->CreateComponent<PlaceholderEditorComponent>();

        entity->Activate();

        // mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        AzToolsFramework::SelectEntity(entityId);

        // move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        EnterComponentMode<PlaceholderEditorComponent>();

        // Component Modes are now instantiated

        // create a simple signal checker type which implements the ComponentModeActionSignalNotificationBus
        const int checkerBusId = 1234;
        ComponentModeActionSignalNotificationChecker checker(checkerBusId);

        // when a shortcut action happens, we want to send a message to the checker bus
        // internally PlaceHolderComponentMode sets up an action to send an event to
        // ComponentModeActionSignalNotifications::OnActionTriggered - we make sure each
        // Component Mode is will sent the notification to the correct address.
        ComponentModeActionSignalRequestBus::Event(
            AZ::EntityComponentIdPair(entityId, placeholder1->GetId()),
            &ComponentModeActionSignalRequests::SetComponentModeActionNotificationBusToNotify, checkerBusId);

        ComponentModeActionSignalRequestBus::Event(
            AZ::EntityComponentIdPair(entityId, placeholder2->GetId()),
            &ComponentModeActionSignalRequests::SetComponentModeActionNotificationBusToNotify, checkerBusId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // trigger the shortcut for this Component Mode
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_Space);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // ensure the checker count is what we expect (both Component Modes will notify the
        // ComponentModeActionSignalNotificationChecker connected at the address specified)
        EXPECT_THAT(checker.GetCount(), Eq(2));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, ShouldIgnoreMouseEventWhenOverridenByComponentMode)
    {
        using OverrideMouseInteractionComponent = TestComponentModeComponent<OverrideMouseInteractionComponentMode>;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // Setup default editor interaction model.
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // Add placeholder component which implements component mode.
        entity->CreateComponent<OverrideMouseInteractionComponent>();

        entity->Activate();

        // Mimic selecting the entity in the viewport (after selection the ComponentModeDelegate
        // connects to the ComponentModeDelegateRequestBus on the entity/component pair address)
        AzToolsFramework::SelectEntity(entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // Move all selected components into ComponentMode
        // (mimic pressing the 'Edit' button to begin Component Mode)
        EnterComponentMode<OverrideMouseInteractionComponent>();

        ViewportInteraction::MouseInteractionEvent interactionEvent;
        interactionEvent.m_mouseEvent = ViewportInteraction::MouseEvent::Move;

        // Simulate a mouse event
        using MouseInteractionResult = AzToolsFramework::ViewportInteraction::MouseInteractionResult;
        MouseInteractionResult handled = MouseInteractionResult::None;
        EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(
            handled, &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions, interactionEvent);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        // Check it was handled by the component mode.
        EXPECT_EQ(handled, MouseInteractionResult::Viewport);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // Test version of EntityPropertyEditor to detect/ensure certain functions were called
    class TestEntityPropertyEditor : public AzToolsFramework::EntityPropertyEditor
    {
    public:
        AZ_CLASS_ALLOCATOR(TestEntityPropertyEditor, AZ::SystemAllocator)
        void InvalidatePropertyDisplay(PropertyModificationRefreshLevel level) override;
        bool m_invalidatePropertyDisplayCalled = false;
    };

    void TestEntityPropertyEditor::InvalidatePropertyDisplay([[maybe_unused]] PropertyModificationRefreshLevel level)
    {
        m_invalidatePropertyDisplayCalled = true;
    }

    // Simple fixture to encapsulate a TestEntityPropertyEditor
    class ComponentModePinnedSelectionFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_testEntityPropertyEditor = AZStd::make_unique<TestEntityPropertyEditor>();
        }

        void TearDownEditorFixtureImpl() override
        {
            m_testEntityPropertyEditor.reset();
        }

        AZStd::unique_ptr<TestEntityPropertyEditor> m_testEntityPropertyEditor;
    };

    TEST_F(ComponentModePinnedSelectionFixture, CannotEnterComponentModeWhenEntityIsPinnedButNotSelected)
    {
        using PlaceHolderComponent = TestComponentModeComponent<PlaceHolderComponentMode>;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        // Add placeholder component which implements component mode.
        entity->CreateComponent<PlaceHolderComponent>();

        AZ_TEST_START_TRACE_SUPPRESSION;
        entity->Activate();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // select entity
        const auto selectedEntities = AzToolsFramework::EntityIdList{ entityId };
        SelectEntities(selectedEntities);

        // pin entity
        AzToolsFramework::EntityIdSet selectedSet(selectedEntities.begin(), selectedEntities.end());
        m_testEntityPropertyEditor->SetOverrideEntityIds(selectedSet);

        // deselect entity
        SelectEntities(AzToolsFramework::EntityIdList{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(m_testEntityPropertyEditor->IsLockedToSpecificEntities());
        EXPECT_TRUE(m_testEntityPropertyEditor->m_invalidatePropertyDisplayCalled);

        bool couldBeginComponentMode = AzToolsFramework::ComponentModeFramework::CouldBeginComponentModeWithEntity(entityId);

        EXPECT_FALSE(couldBeginComponentMode);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ComponentModeTestFixture, CannotEnterComponentModeWhenThereArePendingComponents)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entity = nullptr;
        AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

        entity->Deactivate();

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(
            &AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsToEntities, AzToolsFramework::EntityIdList{ entityId },
            AZ::ComponentTypeList{ AZ::AzTypeInfo<AnotherPlaceholderEditorComponent>::Uuid() });

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(
            &AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsToEntities, AzToolsFramework::EntityIdList{ entityId },
            AZ::ComponentTypeList{ AZ::AzTypeInfo<IncompatiblePlaceholderEditorComponent>::Uuid() });

        entity->Activate();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SelectEntities(AzToolsFramework::EntityIdList{ entityId });
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        AZ::Entity::ComponentArrayType pendingComponents;
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(
            entityId, &AzToolsFramework::EditorPendingCompositionRequestBus::Events::GetPendingComponents, pendingComponents);

        // ensure we do have pending components
        EXPECT_EQ(pendingComponents.size(), 1);
        // cannot enter Component Mode with pending components
        EXPECT_FALSE(AzToolsFramework::ComponentModeFramework::CouldBeginComponentModeWithEntity(entityId));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
} // namespace UnitTest
