/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/ToString.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityModel.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzQtComponents/Components/GlobalEventFilter.h>

using namespace AzToolsFramework;

namespace AZ
{
    std::ostream& operator<<(std::ostream& os, const EntityId entityId)
    {
        return os << entityId.ToString().c_str();
    }
}

namespace UnitTest
{
    class EditorEntityVisibilityCacheFixture
        : public ToolsApplicationFixture
    {
    public:
        void CreateLayerAndEntityHierarchy()
        {
            // Set up entity layer hierarchy.
            const AZ::EntityId a = CreateDefaultEditorEntity("A");
            const AZ::EntityId b = CreateDefaultEditorEntity("B");
            const AZ::EntityId c = CreateDefaultEditorEntity("C");

            m_layerId = CreateEditorLayerEntity("Layer");

            AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
            AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, a);
            AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, b);

            // Add entity ids we want to track, to the visibility cache.
            m_entityIds.insert(m_entityIds.begin(), { a, b, c });
            m_cache.AddEntityIds(m_entityIds);
        }

        EntityIdList m_entityIds;
        AZ::EntityId m_layerId;
        EditorVisibleEntityDataCache m_cache;
    };

    TEST_F(EditorEntityVisibilityCacheFixture, LayerLockAffectsChildEntitiesInEditorEntityCache)
    {
        // Given
        CreateLayerAndEntityHierarchy();

        // Check preconditions.
        EXPECT_FALSE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[0]).value()));
        EXPECT_FALSE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[1]).value()));
        EXPECT_FALSE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[2]).value()));

        // When
        SetEntityLockState(m_layerId, true);

        // Then
        EXPECT_TRUE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[0]).value()));
        EXPECT_TRUE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[1]).value()));
        EXPECT_TRUE(m_cache.IsVisibleEntityLocked(m_cache.GetVisibleEntityIndexFromId(m_entityIds[2]).value()));
    }

    TEST_F(EditorEntityVisibilityCacheFixture, LayerVisibilityAffectsChildEntitiesInEditorEntityCache)
    {
        // Given
        CreateLayerAndEntityHierarchy();

        // Check preconditions.
        EXPECT_TRUE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[0]).value()));
        EXPECT_TRUE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[1]).value()));
        EXPECT_TRUE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[2]).value()));

        // When
        SetEntityVisibility(m_layerId, false);

        // Then
        EXPECT_FALSE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[0]).value()));
        EXPECT_FALSE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[1]).value()));
        EXPECT_FALSE(m_cache.IsVisibleEntityVisible(m_cache.GetVisibleEntityIndexFromId(m_entityIds[2]).value()));
    }

    // Fixture to support testing EditorTransformComponentSelection functionality on an Entity selection.
    class EditorTransformComponentSelectionFixture
        : public ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_entity1 = CreateDefaultEditorEntity("Entity1");
            m_entityIds.push_back(m_entity1);
        }

        void ArrangeIndividualRotatedEntitySelection(const AZ::Quaternion& orientation);
        AZStd::optional<AZ::Transform> GetManipulatorTransform() const;
        void RefreshManipulators(EditorTransformComponentSelectionRequests::RefreshType refreshType);
        void SetTransformMode(EditorTransformComponentSelectionRequests::Mode transformMode);
        void OverrideManipulatorOrientation(const AZ::Quaternion& orientation);
        void OverrideManipulatorTranslation(const AZ::Vector3& translation);

    public:
        AZ::EntityId m_entity1;
        EntityIdList m_entityIds;
    };

    void EditorTransformComponentSelectionFixture::ArrangeIndividualRotatedEntitySelection(
        const AZ::Quaternion& orientation)
    {
        for (auto entityId : m_entityIds)
        {
            AZ::TransformBus::Event(
                entityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, orientation);
        }
    }

    AZStd::optional<AZ::Transform> EditorTransformComponentSelectionFixture::GetManipulatorTransform() const
    {
        AZStd::optional<AZ::Transform> manipulatorTransform;
        EditorTransformComponentSelectionRequestBus::EventResult(
            manipulatorTransform, GetEntityContextId(),
            &EditorTransformComponentSelectionRequests::GetManipulatorTransform);
        return manipulatorTransform;
    }

    void EditorTransformComponentSelectionFixture::RefreshManipulators(
        EditorTransformComponentSelectionRequests::RefreshType refreshType)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::RefreshManipulators, refreshType);
    }

    void EditorTransformComponentSelectionFixture::SetTransformMode(
        EditorTransformComponentSelectionRequests::Mode transformMode)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::SetTransformMode,
            transformMode);
    }

    void EditorTransformComponentSelectionFixture::OverrideManipulatorOrientation(
        const AZ::Quaternion& orientation)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::OverrideManipulatorOrientation,
            orientation);
    }

    void EditorTransformComponentSelectionFixture::OverrideManipulatorTranslation(
        const AZ::Vector3& translation)
    {
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(), &EditorTransformComponentSelectionRequests::OverrideManipulatorTranslation,
            translation);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EditorTransformComponentSelection Tests

    TEST_F(EditorTransformComponentSelectionFixture, ManipulatorOrientationIsResetWhenEntityOrientationIsReset)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AzToolsFramework::SelectEntity(m_entity1);

        ArrangeIndividualRotatedEntitySelection(AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f)));
        RefreshManipulators(EditorTransformComponentSelectionRequests::RefreshType::All);

        SetTransformMode(EditorTransformComponentSelectionRequests::Mode::Rotation);

        const AZ::Transform manipulatorTransformBefore =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check preconditions - manipulator transform matches parent/world transform (identity)
        EXPECT_THAT(manipulatorTransformBefore.GetBasisY(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformBefore.GetBasisZ(), IsClose(AZ::Vector3::CreateAxisZ()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // R - reset entity and manipulator orientation when in Rotation Mode
        QTest::keyPress(&m_editorActions.m_defaultWidget, Qt::Key_R);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        const AZ::Transform manipulatorTransformAfter =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check postconditions - manipulator transform matches parent/world transform (identity)
        EXPECT_THAT(manipulatorTransformAfter.GetBasisY(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformAfter.GetBasisZ(), IsClose(AZ::Vector3::CreateAxisZ()));

        for (auto entityId : m_entityIds)
        {
            // create invalid starting orientation to guarantee correct data is coming from GetLocalRotationQuaternion
            AZ::Quaternion entityOrientation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), 90.0f);
            AZ::TransformBus::EventResult(
                entityOrientation, entityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);

            // manipulator orientation matches entity orientation
            EXPECT_THAT(entityOrientation, IsClose(manipulatorTransformAfter.GetRotation()));
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorTransformComponentSelectionFixture, EntityOrientationRemainsConstantWhenOnlyManipulatorOrientationIsReset)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AzToolsFramework::SelectEntity(m_entity1);

        const AZ::Quaternion initialEntityOrientation = AZ::Quaternion::CreateRotationX(AZ::DegToRad(90.0f));
        ArrangeIndividualRotatedEntitySelection(initialEntityOrientation);

        // assign new orientation to manipulator which does not match entity orientation
        OverrideManipulatorOrientation(AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f)));

        SetTransformMode(EditorTransformComponentSelectionRequests::Mode::Rotation);

        const AZ::Transform manipulatorTransformBefore =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check preconditions - manipulator transform matches manipulator orientation override (not entity transform)
        EXPECT_THAT(manipulatorTransformBefore.GetBasisX(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformBefore.GetBasisY(), IsClose(-AZ::Vector3::CreateAxisX()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // Ctrl+R - reset only manipulator orientation when in Rotation Mode
        QTest::keyPress(&m_editorActions.m_defaultWidget, Qt::Key_R, Qt::ControlModifier);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        const AZ::Transform manipulatorTransformAfter =
            GetManipulatorTransform().value_or(AZ::Transform::CreateIdentity());

        // check postconditions - manipulator transform matches parent/world space (manipulator override was cleared)
        EXPECT_THAT(manipulatorTransformAfter.GetBasisY(), IsClose(AZ::Vector3::CreateAxisY()));
        EXPECT_THAT(manipulatorTransformAfter.GetBasisZ(), IsClose(AZ::Vector3::CreateAxisZ()));

        for (auto entityId : m_entityIds)
        {
            AZ::Quaternion entityOrientation;
            AZ::TransformBus::EventResult(
                entityOrientation, entityId, &AZ::TransformBus::Events::GetLocalRotationQuaternion);

            // entity transform matches initial (entity transform was not reset, only manipulator was)
            EXPECT_THAT(entityOrientation, IsClose(initialEntityOrientation));
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorTransformComponentSelectionFixture, TestComponentPropertyNotificationIsSentAfterModifyingSlice)
    {
        AUTO_RESULT_IF_SETTING_TRUE(UnitTest::prefabSystemSetting, true)

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* grandParent = nullptr;
        AZ::Entity* parent = nullptr;
        AZ::Entity* child = nullptr;

        AZ::EntityId grandParentId = CreateDefaultEditorEntity("GrandParent", &grandParent);
        AZ::EntityId parentId = CreateDefaultEditorEntity("Parent", &parent);
        AZ::EntityId childId = CreateDefaultEditorEntity("Child", &child);

        AZ::TransformBus::Event(
            childId, &AZ::TransformInterface::SetParent, parentId);
        AZ::TransformBus::Event(
            parentId, &AZ::TransformInterface::SetParent, grandParentId);

        UnitTest::SliceAssets sliceAssets;
        const auto sliceAssetId = UnitTest::SaveAsSlice({ grandParent }, GetApplication(), sliceAssets);

        EntityList instantiatedEntities =
            UnitTest::InstantiateSlice(sliceAssetId, sliceAssets);

        const AZ::EntityId entityIdToMove = instantiatedEntities.back()->GetId();
        EditorEntityComponentChangeDetector editorEntityChangeDetector(entityIdToMove);

        AzToolsFramework::SelectEntity(entityIdToMove);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        EditorTransformComponentSelectionRequestBus::Event(
            GetEntityContextId(),
            &EditorTransformComponentSelectionRequests::CopyOrientationToSelectedEntitiesIndividual,
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(90.0f)));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        UnitTest::DestroySlices(sliceAssets);
    }

    TEST_F(EditorTransformComponentSelectionFixture, InvertSelectionIgnoresLockedAndHiddenEntities)
    {
        using ::testing::UnorderedElementsAreArray;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // note: entity1 is created in the fixture setup
        AzToolsFramework::SelectEntity(m_entity1);

        AZ::EntityId entity2 = CreateDefaultEditorEntity("Entity2");
        AZ::EntityId entity3 = CreateDefaultEditorEntity("Entity3");
        AZ::EntityId entity4 = CreateDefaultEditorEntity("Entity4");
        AZ::EntityId entity5 = CreateDefaultEditorEntity("Entity5");
        AZ::EntityId entity6 = CreateDefaultEditorEntity("Entity6");

        SetEntityVisibility(entity2, false);
        SetEntityLockState(entity3, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // 'Invert Selection' shortcut
        QTest::keyPress(&m_editorActions.m_defaultWidget, Qt::Key_I, Qt::ControlModifier | Qt::ShiftModifier);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        AzToolsFramework::EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &ToolsApplicationRequestBus::Events::GetSelectedEntities);

        AzToolsFramework::EntityIdList expectedSelectedEntities = {entity4, entity5, entity6};

        EXPECT_THAT(selectedEntities, UnorderedElementsAreArray(expectedSelectedEntities));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorTransformComponentSelectionFixture, SelectAllIgnoresLockedAndHiddenEntities)
    {
        using ::testing::UnorderedElementsAreArray;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::EntityId entity2 = CreateDefaultEditorEntity("Entity2");
        AZ::EntityId entity3 = CreateDefaultEditorEntity("Entity3");
        AZ::EntityId entity4 = CreateDefaultEditorEntity("Entity4");
        AZ::EntityId entity5 = CreateDefaultEditorEntity("Entity5");
        AZ::EntityId entity6 = CreateDefaultEditorEntity("Entity6");

        SetEntityVisibility(entity5, false);
        SetEntityLockState(entity6, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // 'Select All' shortcut
        QTest::keyPress(&m_editorActions.m_defaultWidget, Qt::Key_A, Qt::ControlModifier);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        AzToolsFramework::EntityIdList selectedEntities;
        ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &ToolsApplicationRequestBus::Events::GetSelectedEntities);

        AzToolsFramework::EntityIdList expectedSelectedEntities = {m_entity1, entity2, entity3, entity4};

        EXPECT_THAT(selectedEntities, UnorderedElementsAreArray(expectedSelectedEntities));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    using EditorTransformComponentSelectionManipulatorTestFixture =
        IndirectCallManipulatorViewportInteractionFixtureMixin<EditorTransformComponentSelectionFixture>;

    TEST_F(EditorTransformComponentSelectionManipulatorTestFixture, CanMoveEntityUsingManipulatorMouseMovement)
    {
        // the initial starting position of the entity (in front and to the left of the camera)
        const auto initialTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // where the entity should end up (in front and to the right of the camera)
        const auto finalTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 0.0f));

        // calculate the position in screen space of the initial position of the entity
        const auto initialPositionScreen =
            AzFramework::WorldToScreen(initialTransformWorld.GetTranslation(), m_cameraState);
        // calculate the position in screen space of the final position of the entity
        const auto finalPositionScreen =
            AzFramework::WorldToScreen(finalTransformWorld.GetTranslation(), m_cameraState);

        // select the entity (this will cause the manipulators to appear in EditorTransformComponentSelection)
        AzToolsFramework::SelectEntity(m_entity1);
        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_entity1, initialTransformWorld);

        // refresh the manipulators so that they update to the position of the entity
        // note: could skip this by selecting the entity after moving it but its useful to have this for reference
        RefreshManipulators(EditorTransformComponentSelectionRequests::RefreshType::All);

        // create an offset along the linear manipulator pointing along the x-axis (perpendicular to the camera view)
        const auto mouseOffsetOnManipulator = AzFramework::ScreenVector(10, 0);
        // store the mouse down position on the manipulator
        const auto mouseDownPosition = initialPositionScreen + mouseOffsetOnManipulator;
        // final position in screen space of the mouse
        const auto mouseMovePosition = finalPositionScreen + mouseOffsetOnManipulator;

        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(mouseDownPosition)
            ->MouseLButtonDown()
            ->MousePosition(mouseMovePosition)
            ->MouseLButtonUp();

        // read back the position of the entity now
        const AZ::Transform finalEntityTransform = AzToolsFramework::GetWorldTransform(m_entity1);

        // ensure final world positions match
        EXPECT_TRUE(finalEntityTransform.IsClose(finalTransformWorld, 0.01f));
    }

    TEST_F(EditorTransformComponentSelectionManipulatorTestFixture, TranslatingEntityWithLinearManipulatorNotifiesOnEntityTransformChanged)
    {
        EditorEntityComponentChangeDetector editorEntityChangeDetector(m_entity1);

        // the initial starting position of the entity (in front and to the left of the camera)
        const auto initialTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, 10.0f, 0.0f));
        // where the entity should end up (in front and to the right of the camera)
        const auto finalTransformWorld = AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 10.0f, 0.0f));

        // calculate the position in screen space of the initial position of the entity
        const auto initialPositionScreen = AzFramework::WorldToScreen(initialTransformWorld.GetTranslation(), m_cameraState);
        // calculate the position in screen space of the final position of the entity
        const auto finalPositionScreen = AzFramework::WorldToScreen(finalTransformWorld.GetTranslation(), m_cameraState);

        // move the entity to its starting position
        AzToolsFramework::SetWorldTransform(m_entity1, initialTransformWorld);
        // select the entity (this will cause the manipulators to appear in EditorTransformComponentSelection)
        AzToolsFramework::SelectEntity(m_entity1);

        // create an offset along the linear manipulator pointing along the x-axis (perpendicular to the camera view)
        const auto mouseOffsetOnManipulator = AzFramework::ScreenVector(10, 0);
        // store the mouse down position on the manipulator
        const auto mouseDownPosition = initialPositionScreen + mouseOffsetOnManipulator;
        // final position in screen space of the mouse
        const auto mouseMovePosition = finalPositionScreen + mouseOffsetOnManipulator;

        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(mouseDownPosition)
            ->MouseLButtonDown()
            ->MousePosition(mouseMovePosition)
            ->MouseLButtonUp();

        // verify a EditorTransformChangeNotificationBus::OnEntityTransformChanged occurred
        using ::testing::UnorderedElementsAreArray;
        EXPECT_THAT(editorEntityChangeDetector.m_entityIds, UnorderedElementsAreArray(m_entityIds));
    }

    // simple widget to listen for a mouse wheel event and then forward it on to the ViewportSelectionRequestBus
    class WheelEventWidget
        : public QWidget
    {
        using MouseInteractionResult = AzToolsFramework::ViewportInteraction::MouseInteractionResult;
    public:
        WheelEventWidget(QWidget* parent = nullptr)
            : QWidget(parent)
        {
        }

        void wheelEvent(QWheelEvent* ev) override
        {
            namespace vi = AzToolsFramework::ViewportInteraction;
            vi::MouseInteraction mouseInteraction;
            mouseInteraction.m_interactionId.m_cameraId = AZ::EntityId();
            mouseInteraction.m_interactionId.m_viewportId = 0;
            mouseInteraction.m_mouseButtons = vi::BuildMouseButtons(ev->buttons());
            mouseInteraction.m_mousePick = vi::MousePick();
            mouseInteraction.m_keyboardModifiers = vi::BuildKeyboardModifiers(ev->modifiers());

            AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::EventResult(
                m_mouseInteractionResult, AzToolsFramework::GetEntityContextId(),
                &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleAllMouseInteractions,
                vi::MouseInteractionEvent(mouseInteraction, ev->angleDelta().y()));
        }

        MouseInteractionResult m_mouseInteractionResult;
    };

    TEST_F(EditorTransformComponentSelectionFixture, MouseScrollWheelSwitchesTransformMode)
    {
        using ::testing::Eq;
        namespace vi = AzToolsFramework::ViewportInteraction;

        const auto transformMode = []()
        {
            EditorTransformComponentSelectionRequests::Mode transformMode;
            EditorTransformComponentSelectionRequestBus::EventResult(
                transformMode, GetEntityContextId(),
                &EditorTransformComponentSelectionRequestBus::Events::GetTransformMode);
            return transformMode;
        };

        // given
        // preconditions
        EXPECT_THAT(transformMode(), EditorTransformComponentSelectionRequests::Mode::Translation);

        auto wheelEventWidget = WheelEventWidget();
        // attach the global event filter to the placeholder widget
        AzQtComponents::GlobalEventFilter globalEventFilter(QApplication::instance());
        wheelEventWidget.installEventFilter(&globalEventFilter);

        // example mouse wheel event (does not yet factor in position of mouse in relation to widget)
        auto wheelEvent = QWheelEvent(
            QPointF(0.0f, 0.0f), QPointF(0.0f, 0.0f), QPoint(0, 1), QPoint(0, 0), Qt::MouseButton::NoButton,
            Qt::KeyboardModifier::ControlModifier, Qt::ScrollPhase::ScrollBegin, false,
            Qt::MouseEventSource::MouseEventSynthesizedBySystem);

        // when (trigger mouse wheel event)
        QApplication::sendEvent(&wheelEventWidget, &wheelEvent);

        // then
        // transform mode has changed and mouse event was handled
        EXPECT_THAT(transformMode(), Eq(EditorTransformComponentSelectionRequests::Mode::Rotation));
        EXPECT_THAT(wheelEventWidget.m_mouseInteractionResult, Eq(vi::MouseInteractionResult::Viewport));
    }

    // struct to contain input reference frame and expected orientation outcome based on
    // the reference frame, selection and entity hierarchy
    struct ReferenceFrameWithOrientation
    {
        ReferenceFrame m_referenceFrame; // the input reference frame (Local/Parent/World)
        AZ::Quaternion m_orientation; // the orientation of the manipulator transform
    };

    // custom orientation to compare against for leaf/child entities (when ReferenceFrame is Local)
    static const AZ::Quaternion ChildExpectedPivotLocalOrientationInWorldSpace =
        AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::DegToRad(45.0f));

    // custom orientation to compare against for branch/parent entities (when ReferenceFrame is Parent)
    static const AZ::Quaternion ParentExpectedPivotLocalOrientationInWorldSpace =
        AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::DegToRad(45.0f));

    // custom orientation to compare against for orientation/pivot override
    static const AZ::Quaternion PivotOverrideLocalOrientationInWorldSpace =
        AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::DegToRad(90.0f));

    class EditorTransformComponentSelectionSingleEntityPivotFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(EditorTransformComponentSelectionSingleEntityPivotFixture, PivotOrientationMatchesReferenceFrameSingleEntity)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculatePivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ChildExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateZero()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        const PivotOrientationResult pivotResult =
            CalculatePivotOrientation(m_entityIds[0], referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionSingleEntityPivotFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ReferenceFrame::Local, ChildExpectedPivotLocalOrientationInWorldSpace},
            ReferenceFrameWithOrientation{ReferenceFrame::Parent, AZ::Quaternion::CreateIdentity()},
            ReferenceFrameWithOrientation{ReferenceFrame::World, AZ::Quaternion::CreateIdentity()}));

    class EditorTransformComponentSelectionSingleEntityWithParentPivotFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionSingleEntityWithParentPivotFixture,
        PivotOrientationMatchesReferenceFrameEntityWithParent)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculatePivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::EntityId parentEntityId = CreateDefaultEditorEntity("Parent");
        AZ::TransformBus::Event(m_entityIds[0], &AZ::TransformBus::Events::SetParent, parentEntityId);

        AZ::TransformBus::Event(
            parentEntityId, &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ParentExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateZero()));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ChildExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateAxisZ(-5.0f)));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        const PivotOrientationResult pivotResult =
            CalculatePivotOrientation(m_entityIds[0], referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // with a single entity selected with a parent the orientation reference frames follow as you'd expect
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionSingleEntityWithParentPivotFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ReferenceFrame::Local, ChildExpectedPivotLocalOrientationInWorldSpace},
            ReferenceFrameWithOrientation{ReferenceFrame::Parent, ParentExpectedPivotLocalOrientationInWorldSpace},
            ReferenceFrameWithOrientation{ReferenceFrame::World, AZ::Quaternion::CreateIdentity()}));

    class EditorTransformComponentSelectionMultipleEntitiesPivotFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesPivotFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntities)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculatePivotOrientationForEntityIds;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));

        // setup entities in arbitrary triangle arrangement
        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(-10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        // note: EntityIdManipulatorLookup{} is unused during this test
        EntityIdManipulatorLookups lookups {
            {m_entityIds[0], EntityIdManipulatorLookup{}},
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        const PivotOrientationResult pivotResult =
            CalculatePivotOrientationForEntityIds(lookups, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // with a group selection, when the entities are not in a hierarchy, no matter what reference frame,
    // we will always get an orientation aligned to the world
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesPivotFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionMultipleEntitiesWithSameParentPivotFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesWithSameParentPivotFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntitiesSameParent)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculatePivotOrientationForEntityIds;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ParentExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateAxisZ(-5.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        AZ::TransformBus::Event(m_entityIds[1], &AZ::TransformBus::Events::SetParent, m_entityIds[0]);
        AZ::TransformBus::Event(m_entityIds[2], &AZ::TransformBus::Events::SetParent, m_entityIds[0]);

        // note: EntityIdManipulatorLookup{} is unused during this test
        // only select second two entities that are children of m_entityIds[0]
        EntityIdManipulatorLookups lookups{
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        const PivotOrientationResult pivotResult =
            CalculatePivotOrientationForEntityIds(lookups, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // here two entities are selected with the same parent - local and parent will match parent space, with world
    // giving the identity (aligned to world axes)
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesWithSameParentPivotFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, ParentExpectedPivotLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, ParentExpectedPivotLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionMultipleEntitiesWithDifferentParentPivotFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesWithDifferentParentPivotFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntitiesDifferentParent)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculatePivotOrientationForEntityIds;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity4"));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ParentExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateAxisZ(-5.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        AZ::TransformBus::Event(m_entityIds[1], &AZ::TransformBus::Events::SetParent, m_entityIds[0]);
        AZ::TransformBus::Event(m_entityIds[2], &AZ::TransformBus::Events::SetParent, m_entityIds[3]);

        // note: EntityIdManipulatorLookup{} is unused during this test
        // only select second two entities that are children of different m_entities
        EntityIdManipulatorLookups lookups{
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        const PivotOrientationResult pivotResult =
            CalculatePivotOrientationForEntityIds(lookups, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // if multiple entities are selected without a parent in common, orientation will always be world again
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesWithDifferentParentPivotFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionSingleEntityPivotAndOverrideFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionSingleEntityPivotAndOverrideFixture,
        PivotOrientationMatchesReferenceFrameSingleEntityOptionalOverride)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculateSelectionPivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ChildExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateZero()));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        EntityIdManipulatorLookups lookups{
            {m_entityIds[0], EntityIdManipulatorLookup{}}
        };

        // set override frame (orientation only)
        OptionalFrame optionalFrame;
        optionalFrame.m_orientationOverride = PivotOverrideLocalOrientationInWorldSpace;

        const PivotOrientationResult pivotResult =
            CalculateSelectionPivotOrientation(lookups, optionalFrame, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // local reference frame will still return local orientation for entity, but pivot override will trump parent
    // space (world will still give identity alignment for axes)
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionSingleEntityPivotAndOverrideFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, ChildExpectedPivotLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, PivotOverrideLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionMultipleEntitiesPivotAndOverrideFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesPivotAndOverrideFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntitiesOptionalOverride)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculateSelectionPivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(-10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        // note: EntityIdManipulatorLookup{} is unused during this test
        EntityIdManipulatorLookups lookups{
            {m_entityIds[0], EntityIdManipulatorLookup{}},
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        OptionalFrame optionalFrame;
        optionalFrame.m_orientationOverride = PivotOverrideLocalOrientationInWorldSpace;

        const PivotOrientationResult pivotResult =
            CalculateSelectionPivotOrientation(lookups, optionalFrame, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // with multiple entities selected, override frame wins in both local and parent reference frames
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesPivotAndOverrideFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, PivotOverrideLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, PivotOverrideLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionMultipleEntitiesPivotAndNoOverrideFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesPivotAndNoOverrideFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntitiesNoOptionalOverride)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculateSelectionPivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(-10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        // note: EntityIdManipulatorLookup{} is unused during this test
        EntityIdManipulatorLookups lookups{
            {m_entityIds[0], EntityIdManipulatorLookup{}},
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        OptionalFrame optionalFrame;
        const PivotOrientationResult pivotResult =
            CalculateSelectionPivotOrientation(lookups, optionalFrame, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // multiple entities selected (no hierarchy) always get world aligned axes (identity)
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesPivotAndNoOverrideFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, AZ::Quaternion::CreateIdentity() },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorTransformComponentSelectionMultipleEntitiesSameParentPivotAndNoOverrideFixture
        : public EditorTransformComponentSelectionFixture
        , public ::testing::WithParamInterface<ReferenceFrameWithOrientation> {};

    TEST_P(
        EditorTransformComponentSelectionMultipleEntitiesSameParentPivotAndNoOverrideFixture,
        PivotOrientationMatchesReferenceFrameMultipleEntitiesSameParentNoOptionalOverride)
    {
        using ETCS::PivotOrientationResult;
        using ETCS::CalculateSelectionPivotOrientation;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity2"));
        m_entityIds.push_back(CreateDefaultEditorEntity("Entity3"));

        AZ::TransformBus::Event(
            m_entityIds[0], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                ParentExpectedPivotLocalOrientationInWorldSpace, AZ::Vector3::CreateAxisZ(-5.0f)));

        AZ::TransformBus::Event(
            m_entityIds[1], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisX(10.0f)));

        AZ::TransformBus::Event(
            m_entityIds[2], &AZ::TransformBus::Events::SetWorldTM,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f)));

        AZ::TransformBus::Event(m_entityIds[1], &AZ::TransformBus::Events::SetParent, m_entityIds[0]);
        AZ::TransformBus::Event(m_entityIds[2], &AZ::TransformBus::Events::SetParent, m_entityIds[0]);

        // note: EntityIdManipulatorLookup{} is unused during this test
        EntityIdManipulatorLookups lookups{
            {m_entityIds[1], EntityIdManipulatorLookup{}},
            {m_entityIds[2], EntityIdManipulatorLookup{}}
        };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        const ReferenceFrameWithOrientation referenceFrameWithOrientation = GetParam();

        OptionalFrame optionalFrame;
        const PivotOrientationResult pivotResult =
            CalculateSelectionPivotOrientation(lookups, optionalFrame, referenceFrameWithOrientation.m_referenceFrame);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(pivotResult.m_worldOrientation, IsClose(referenceFrameWithOrientation.m_orientation));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // no optional frame, same parent, local and parent both get parent alignment (world reference frame
    // gives world alignment (identity))
    INSTANTIATE_TEST_CASE_P(
        All,
        EditorTransformComponentSelectionMultipleEntitiesSameParentPivotAndNoOverrideFixture,
        testing::Values(
            ReferenceFrameWithOrientation{ ReferenceFrame::Local, ParentExpectedPivotLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::Parent, ParentExpectedPivotLocalOrientationInWorldSpace },
            ReferenceFrameWithOrientation{ ReferenceFrame::World, AZ::Quaternion::CreateIdentity() }));

    class EditorEntityModelVisibilityFixture
        : public ToolsApplicationFixture
        , private EditorEntityVisibilityNotificationBus::Router
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
            EditorEntityInfoNotificationBus::Handler::BusConnect();
        }

        void TearDownEditorFixtureImpl() override
        {
            EditorEntityInfoNotificationBus::Handler::BusDisconnect();
            EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
        }

        bool m_entityInfoUpdatedVisibilityForLayer = false;
        AZ::EntityId m_layerId;

    private:
        // EditorEntityVisibilityNotificationBus ...
        void OnEntityVisibilityChanged(bool /*visibility*/) override
        {
            // for debug purposes
        }

        // EditorEntityInfoNotificationBus ...
        void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool /*visible*/) override
        {
            if (entityId == m_layerId)
            {
                m_entityInfoUpdatedVisibilityForLayer = true;
            }
        }
    };

    // all entities in a layer are the same state, modifying the layer
    // will also notify the UI to refresh
    TEST_F(EditorEntityModelVisibilityFixture, LayerVisibilityNotifiesEditorEntityModelState)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::EntityId a = CreateDefaultEditorEntity("A");
        const AZ::EntityId b = CreateDefaultEditorEntity("B");
        const AZ::EntityId c = CreateDefaultEditorEntity("C");

        m_layerId = CreateEditorLayerEntity("Layer");

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(a, false);
        SetEntityVisibility(b, false);
        SetEntityVisibility(c, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(IsEntityVisible(a));
        EXPECT_FALSE(IsEntityVisible(b));
        EXPECT_FALSE(IsEntityVisible(c));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(m_layerId, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(IsEntityVisible(m_layerId));
        EXPECT_TRUE(m_entityInfoUpdatedVisibilityForLayer);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // reset property
        m_entityInfoUpdatedVisibilityForLayer = false;

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(m_layerId, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(m_entityInfoUpdatedVisibilityForLayer);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorEntityModelVisibilityFixture, UnhidingEntityInInvisibleLayerUnhidesAllEntitiesThatWereNotIndividuallyHidden)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::EntityId a = CreateDefaultEditorEntity("A");
        const AZ::EntityId b = CreateDefaultEditorEntity("B");
        const AZ::EntityId c = CreateDefaultEditorEntity("C");

        const AZ::EntityId d = CreateDefaultEditorEntity("D");
        const AZ::EntityId e = CreateDefaultEditorEntity("E");
        const AZ::EntityId f = CreateDefaultEditorEntity("F");

        m_layerId = CreateEditorLayerEntity("Layer1");
        const AZ::EntityId secondLayerId = CreateEditorLayerEntity("Layer2");

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);

        AZ::TransformBus::Event(secondLayerId, &AZ::TransformBus::Events::SetParent, m_layerId);

        AZ::TransformBus::Event(d, &AZ::TransformBus::Events::SetParent, secondLayerId);
        AZ::TransformBus::Event(e, &AZ::TransformBus::Events::SetParent, secondLayerId);
        AZ::TransformBus::Event(f, &AZ::TransformBus::Events::SetParent, secondLayerId);

        // Layer1
          // A
          // B
          // C
          // Layer2
            // D
            // E
            // F

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // hide top layer
        SetEntityVisibility(m_layerId, false);

        // hide a and c (a and see are 'set' not to be visible and are not visible)
        SetEntityVisibility(a, false);
        SetEntityVisibility(c, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(!IsEntityVisible(a));
        EXPECT_TRUE(!IsEntitySetToBeVisible(a));

        // b will not be visible but is not 'set' to be hidden
        EXPECT_TRUE(!IsEntityVisible(b));
        EXPECT_TRUE(IsEntitySetToBeVisible(b));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // same for nested layer
        SetEntityVisibility(secondLayerId, false);

        SetEntityVisibility(d, false);
        SetEntityVisibility(f, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(!IsEntityVisible(e));
        EXPECT_TRUE(IsEntitySetToBeVisible(e));

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // set visibility of most nested entity to true
        SetEntityVisibility(d, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(IsEntitySetToBeVisible(m_layerId));
        EXPECT_TRUE(IsEntitySetToBeVisible(secondLayerId));

        // a will still be set to be not visible and won't be visible as parent layer is now visible
        EXPECT_TRUE(!IsEntitySetToBeVisible(a));
        EXPECT_TRUE(!IsEntityVisible(a));

        // b will now be visible as it was not individually
        // set to be visible and the parent layer is now visible
        EXPECT_TRUE(IsEntitySetToBeVisible(b));
        EXPECT_TRUE(IsEntityVisible(b));

        // same story for e as for b
        EXPECT_TRUE(IsEntitySetToBeVisible(e));
        EXPECT_TRUE(IsEntityVisible(e));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorEntityModelVisibilityFixture, UnlockingEntityInLockedLayerUnlocksAllEntitiesThatWereNotIndividuallyLocked)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::EntityId a = CreateDefaultEditorEntity("A");
        const AZ::EntityId b = CreateDefaultEditorEntity("B");
        const AZ::EntityId c = CreateDefaultEditorEntity("C");

        const AZ::EntityId d = CreateDefaultEditorEntity("D");
        const AZ::EntityId e = CreateDefaultEditorEntity("E");
        const AZ::EntityId f = CreateDefaultEditorEntity("F");

        m_layerId = CreateEditorLayerEntity("Layer1");
        const AZ::EntityId secondLayerId = CreateEditorLayerEntity("Layer2");

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);

        AZ::TransformBus::Event(secondLayerId, &AZ::TransformBus::Events::SetParent, m_layerId);

        AZ::TransformBus::Event(d, &AZ::TransformBus::Events::SetParent, secondLayerId);
        AZ::TransformBus::Event(e, &AZ::TransformBus::Events::SetParent, secondLayerId);
        AZ::TransformBus::Event(f, &AZ::TransformBus::Events::SetParent, secondLayerId);

        // Layer1
          // A
          // B
          // C
          // Layer2
            // D
            // E
            // F

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // lock top layer
        SetEntityLockState(m_layerId, true);

        // lock a and c (a and see are 'set' not to be visible and are not visible)
        SetEntityLockState(a, true);
        SetEntityLockState(c, true);

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(IsEntityLocked(a));
        EXPECT_TRUE(IsEntitySetToBeLocked(a));

        // b will be locked but is not 'set' to be locked
        EXPECT_TRUE(IsEntityLocked(b));
        EXPECT_TRUE(!IsEntitySetToBeLocked(b));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // same for nested layer
        SetEntityLockState(secondLayerId, true);

        SetEntityLockState(d, true);
        SetEntityLockState(f, true);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(IsEntityLocked(e));
        EXPECT_TRUE(!IsEntitySetToBeLocked(e));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // set visibility of most nested entity to true
        SetEntityLockState(d, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(!IsEntitySetToBeLocked(m_layerId));
        EXPECT_TRUE(!IsEntitySetToBeLocked(secondLayerId));

        // a will still be set to be not visible and won't be visible as parent layer is now visible
        EXPECT_TRUE(IsEntitySetToBeLocked(a));
        EXPECT_TRUE(IsEntityLocked(a));

        // b will now be visible as it was not individually
        // set to be visible and the parent layer is now visible
        EXPECT_TRUE(!IsEntitySetToBeLocked(b));
        EXPECT_TRUE(!IsEntityLocked(b));

        // same story for e as for b
        EXPECT_TRUE(!IsEntitySetToBeLocked(e));
        EXPECT_TRUE(!IsEntityLocked(e));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // test to ensure the visibility flag on a layer entity is not modified
    // instead we rely on SetLayerChildrenVisibility and AreLayerChildrenVisible
    TEST_F(EditorEntityModelVisibilityFixture, LayerEntityVisibilityFlagIsNotModified)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const AZ::EntityId a = CreateDefaultEditorEntity("A");
        const AZ::EntityId b = CreateDefaultEditorEntity("B");
        const AZ::EntityId c = CreateDefaultEditorEntity("C");

        m_layerId = CreateEditorLayerEntity("Layer1");

        AZ::TransformBus::Event(a, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(b, &AZ::TransformBus::Events::SetParent, m_layerId);
        AZ::TransformBus::Event(c, &AZ::TransformBus::Events::SetParent, m_layerId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        SetEntityVisibility(m_layerId, false);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(!IsEntitySetToBeVisible(m_layerId));
        EXPECT_TRUE(!IsEntityVisible(m_layerId));

        bool flagSetVisible = false;
        EditorVisibilityRequestBus::EventResult(
            flagSetVisible, m_layerId, &EditorVisibilityRequestBus::Events::GetVisibilityFlag);

        // even though a layer is set to not be visible, this is recorded by SetLayerChildrenVisibility
        // and AreLayerChildrenVisible - the visibility flag will not be modified and remains true
        EXPECT_TRUE(flagSetVisible);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    class EditorEntityInfoRequestActivateTestComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorEntityInfoRequestActivateTestComponent, "{849DA1FC-6A0C-4CB8-A0BB-D90DEE7FF7F7}",
            AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override
        {
            // ensure we can successfully read IsVisible and IsLocked (bus will be connected to in entity Init)
            EditorEntityInfoRequestBus::EventResult(
                m_visible, GetEntityId(), &EditorEntityInfoRequestBus::Events::IsVisible);
            EditorEntityInfoRequestBus::EventResult(
                m_locked, GetEntityId(), &EditorEntityInfoRequestBus::Events::IsLocked);
        }

        void Deactivate() override {}

        bool m_visible = false;
        bool m_locked = true;
    };

    void EditorEntityInfoRequestActivateTestComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorEntityInfoRequestActivateTestComponent>()
                ->Version(0)
                ;
        }
    }

    class EditorEntityModelEntityInfoRequestFixture
        : public ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            GetApplication()->RegisterComponentDescriptor(EditorEntityInfoRequestActivateTestComponent::CreateDescriptor());
        }
    };

    TEST_F(EditorEntityModelEntityInfoRequestFixture, EditorEntityInfoRequestBusRespondsInComponentActivate)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entity = nullptr;
        const AZ::EntityId entityId = CreateDefaultEditorEntity("Entity", &entity);

        entity->Deactivate();
        const auto* entityInfoComponent = entity->CreateComponent<EditorEntityInfoRequestActivateTestComponent>();

        // This is necessary to prevent a warning in the undo system.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            entity->GetId());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        entity->Activate();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(entityInfoComponent->m_visible);
        EXPECT_FALSE(entityInfoComponent->m_locked);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorEntityModelEntityInfoRequestFixture, EditorEntityInfoRequestBusRespondsInComponentActivateInLayer)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        AZ::Entity* entity = nullptr;
        const AZ::EntityId entityId = CreateDefaultEditorEntity("Entity", &entity);
        const AZ::EntityId layerId = CreateEditorLayerEntity("Layer");

        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetParent, layerId);

        SetEntityVisibility(layerId, false);
        SetEntityLockState(layerId, true);

        entity->Deactivate();
        auto* entityInfoComponent = entity->CreateComponent<EditorEntityInfoRequestActivateTestComponent>();

        // This is necessary to prevent a warning in the undo system.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            entity->GetId());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // invert initial state to be sure we know Activate does what it's supposed to
        entityInfoComponent->m_visible = true;
        entityInfoComponent->m_locked = false;
        entity->Activate();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(entityInfoComponent->m_visible);
        EXPECT_TRUE(entityInfoComponent->m_locked);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

} // namespace UnitTest
