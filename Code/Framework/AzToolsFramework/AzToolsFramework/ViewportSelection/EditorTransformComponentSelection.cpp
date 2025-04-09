/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorTransformComponentSelection.h"

#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/Commands/EntityManipulatorCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/ComponentMode/ComponentModeSwitcher.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Manipulators/AngularManipulatorCircleViewFeedback.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <Entity/EditorEntityContextBus.h>
#include <Entity/EditorEntityHelpers.h>
#include <QApplication>
#include <QRect>

static constexpr AZStd::string_view TransformModeChangedUpdaterIdentifier = "o3de.updater.onTransformModeChanged";


namespace AzToolsFramework
{
    namespace EditorTransformComponentSelectionInternal
    {
        // The EditorTransformComponentSelection is a singleton but
        // gets destroyed and created repeatedly.
        // It is responsible for menu items and hotkeys that are global to the editor.
        // This function is used to ensure that the menu items and hotkey callbacks can tell when
        // this object is alive and can get to it if it is, or disable them if it is not, since you only
        // get one shot (at the beginning of the app) to register them.

        static AZ::EnvironmentVariable<EditorTransformComponentSelection*> g_globalInstance;
        static const char* s_globalInstanceName = "EditorTransformComponentSelection";

        EditorTransformComponentSelection* GetCurrentInstance()
        {
            if (!g_globalInstance)
            {
                g_globalInstance = AZ::Environment::FindVariable<EditorTransformComponentSelection*>(s_globalInstanceName);
            }

            return g_globalInstance ? (*g_globalInstance) : nullptr;
        }

        void SetCurrentInstance(EditorTransformComponentSelection* instance)
        {
            if (!g_globalInstance)
            {
                g_globalInstance = AZ::Environment::CreateVariable<EditorTransformComponentSelection*>(s_globalInstanceName);
                (*g_globalInstance) = nullptr;
            }

            (*g_globalInstance) = instance;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(EditorTransformComponentSelection, AZ::SystemAllocator)

    AZ_CVAR(
        float,
        ed_viewportGizmoAxisLineWidth,
        4.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The width of the line for the viewport axis gizmo");
    AZ_CVAR(
        float,
        ed_viewportGizmoAxisLineLength,
        0.7f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The length of the line for the viewport axis gizmo");
    AZ_CVAR(
        float,
        ed_viewportGizmoAxisLabelOffset,
        1.15f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The offset of the label for the viewport axis gizmo");
    AZ_CVAR(
        float,
        ed_viewportGizmoAxisLabelSize,
        1.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The size of each label for the viewport axis gizmo");
    AZ_CVAR(
        AZ::Vector2,
        ed_viewportGizmoAxisScreenPosition,
        AZ::Vector2(0.045f, 0.9f),
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The screen position of the gizmo in normalized (0-1) ndc space");
    AZ_CVAR(
        bool,
        ed_viewportDisplayManipulatorPosition,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Display the position of the manipulator to the viewport as debug text");

    // strings related to new viewport interaction model (EditorTransformComponentSelection)
    static const char* const TogglePivotTitleEditMenu = "Toggle Pivot Location";
    static const char* const TogglePivotDesc = "Toggle pivot location";
    static const char* const ManipulatorUndoRedoName = "Manipulator Adjustment";
    static const char* const LockSelectionTitle = "Lock Selection";
    static const char* const LockSelectionDesc = "Lock the selected entities so that they can't be selected in the viewport";
    static const char* const UnlockSelectionTitle = "Unlock Selection";
    static const char* const UnlockSelectionDesc = "Unlock the selected entities so that they can be selected in the viewport";
    static const char* const HideSelectionTitle = "Hide Selection";
    static const char* const HideSelectionDesc = "Hide the selected entities so that they don't appear in the viewport";
    static const char* const ShowSelectionTitle = "Show Selection";
    static const char* const ShowSelectionDesc = "Show the selected entities so that they appear in the viewport";
    static const char* const UnlockAllTitle = "Unlock All Entities";
    static const char* const UnlockAllDesc = "Unlock all entities the level";
    static const char* const ShowAllTitle = "Show All";
    static const char* const ShowAllDesc = "Show all entities so that they appear in the viewport";
    static const char* const SelectAllTitle = "Select All";
    static const char* const SelectAllDesc = "Select all entities";
    static const char* const InvertSelectionTitle = "Invert Selection";
    static const char* const InvertSelectionDesc = "Invert the current entity selection";
    static const char* const DuplicateTitle = "Duplicate";
    static const char* const DuplicateDesc = "Duplicate selected entities";
    static const char* const DeleteTitle = "Delete";
    static const char* const DeleteDesc = "Delete selected entities";
    static const char* const ResetEntityTransformTitle = "Reset Entity Transform";
    static const char* const ResetEntityTransformDesc = "Reset transform based on manipulator mode";
    static const char* const ResetManipulatorTitle = "Reset Manipulator";
    static const char* const ResetManipulatorDesc = "Reset the manipulator to recenter it on the selected entity";

    static const char* const EntityBoxSelectUndoRedoDesc = "Box Select Entities";
    static const char* const EntityDeselectUndoRedoDesc = "Deselect Entity";
    static const char* const EntitiesDeselectUndoRedoDesc = "Deselect Entities";
    static const char* const ChangeEntitySelectionUndoRedoDesc = "Change Selected Entity";
    static const char* const EntitySelectUndoRedoDesc = "Select Entity";
    static const char* const DittoManipulatorUndoRedoDesc = "Ditto Manipulator";
    static const char* const ResetManipulatorTranslationUndoRedoDesc = "Reset Manipulator Translation";
    static const char* const ResetManipulatorOrientationUndoRedoDesc = "Reset Manipulator Orientation";
    static const char* const DittoEntityOrientationIndividualUndoRedoDesc = "Ditto orientation individual";
    static const char* const DittoEntityOrientationGroupUndoRedoDesc = "Ditto orientation group";
    static const char* const ResetTranslationToParentUndoRedoDesc = "Reset translation to parent";
    static const char* const ResetOrientationToParentUndoRedoDesc = "Reset orientation to parent";
    static const char* const DittoTranslationGroupUndoRedoDesc = "Ditto translation group";
    static const char* const DittoTranslationIndividualUndoRedoDesc = "Ditto translation individual";
    static const char* const DittoScaleIndividualWorldUndoRedoDesc = "Ditto scale individual world";
    static const char* const DittoScaleIndividualLocalUndoRedoDesc = "Ditto scale individual local";
    static const char* const SnapToWorldGridUndoRedoDesc = "Snap to world grid";
    static const char* const ShowAllEntitiesUndoRedoDesc = ShowAllTitle;
    static const char* const LockSelectionUndoRedoDesc = LockSelectionTitle;
    static const char* const HideSelectionUndoRedoDesc = HideSelectionTitle;
    static const char* const UnlockAllUndoRedoDesc = UnlockAllTitle;
    static const char* const SelectAllEntitiesUndoRedoDesc = SelectAllTitle;
    static const char* const InvertSelectionUndoRedoDesc = InvertSelectionTitle;
    static const char* const DuplicateUndoRedoDesc = DuplicateTitle;
    static const char* const DeleteUndoRedoDesc = DeleteTitle;

    static const char* const TransformModeClusterTranslateTooltip = "Switch to translate mode (1)";
    static const char* const TransformModeClusterRotateTooltip = "Switch to rotate mode (2)";
    static const char* const TransformModeClusterScaleTooltip = "Switch to scale mode (3)";
    static const char* const SpaceClusterWorldTooltip = "Toggle world space lock";
    static const char* const SpaceClusterParentTooltip = "Toggle parent space lock";
    static const char* const SpaceClusterLocalTooltip = "Toggle local space lock";
    static const char* const SnappingClusterSnapToWorldTooltip = "Snap selected entities to the world space grid";

    static const AZ::Color FadedXAxisColor = AZ::Color(AZ::u8(200), AZ::u8(127), AZ::u8(127), AZ::u8(255));
    static const AZ::Color FadedYAxisColor = AZ::Color(AZ::u8(127), AZ::u8(190), AZ::u8(127), AZ::u8(255));
    static const AZ::Color FadedZAxisColor = AZ::Color(AZ::u8(120), AZ::u8(120), AZ::u8(180), AZ::u8(255));

    static const AZ::Color PickedOrientationColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    static const AZ::Color SelectedEntityAabbColor = AZ::Color(0.6f, 0.6f, 0.6f, 0.4f);

    static const float PivotSize = 0.075f; // the size of the pivot (box) to render when selected

    // data passed to manipulators when processing mouse interactions
    // m_entityIds should be sorted based on the entity hierarchy
    // (see SortEntitiesByLocationInHierarchy and BuildSortedEntityIdVectorFromEntityIdContainer)
    struct ManipulatorEntityIds
    {
        EntityIdList m_entityIds;
    };

    bool OptionalFrame::HasTranslationOverride() const
    {
        return m_translationOverride.has_value();
    }

    bool OptionalFrame::HasOrientationOverride() const
    {
        return m_orientationOverride.has_value();
    }

    bool OptionalFrame::HasTransformOverride() const
    {
        return m_translationOverride.has_value() || m_orientationOverride.has_value();
    }

    void OptionalFrame::Reset()
    {
        m_orientationOverride.reset();
        m_translationOverride.reset();
    }

    void OptionalFrame::ResetPickedTranslation()
    {
        m_translationOverride.reset();
    }

    void OptionalFrame::ResetPickedOrientation()
    {
        m_orientationOverride.reset();
    }

    using EntityIdTransformMap = AZStd::unordered_map<AZ::EntityId, AZ::Transform>;

    // wrap input/controls for EditorTransformComponentSelection to
    // make visibility clearer and adjusting them easier.
    namespace Input
    {
        static bool CycleManipulator(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Wheel &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool DeselectAll(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left();
        }

        static bool GroupDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool IndividualDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool SnapSurface(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Shift() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool ManipulatorDitto(
            const AzFramework::ClickDetector::ClickOutcome clickOutcome, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return clickOutcome == AzFramework::ClickDetector::ClickOutcome::Click &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
        }

        static bool ManipulatorDittoExact(
            const AzFramework::ClickDetector::ClickOutcome clickOutcome, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return clickOutcome == AzFramework::ClickDetector::ClickOutcome::Click &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Shift();
        }

        static bool IndividualSelect(const AzFramework::ClickDetector::ClickOutcome clickOutcome)
        {
            return clickOutcome == AzFramework::ClickDetector::ClickOutcome::Click;
        }

        static bool AdditiveIndividualSelect(
            const AzFramework::ClickDetector::ClickOutcome clickOutcome, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return clickOutcome == AzFramework::ClickDetector::ClickOutcome::Click &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
                !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() &&
                !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Shift();
        }
    } // namespace Input

    static void DestroyManipulators(EntityIdManipulators& manipulators)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (manipulators.m_manipulators)
        {
            manipulators.m_lookups.clear();
            manipulators.m_manipulators->Unregister();
            manipulators.m_manipulators.reset();
        }
    }

    static EditorTransformComponentSelectionRequests::Pivot TogglePivotMode(const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        switch (pivot)
        {
        case EditorTransformComponentSelectionRequests::Pivot::Object:
            return EditorTransformComponentSelectionRequests::Pivot::Center;
        case EditorTransformComponentSelectionRequests::Pivot::Center:
            return EditorTransformComponentSelectionRequests::Pivot::Object;
        default:
            AZ_Assert(false, "Invalid Pivot");
            return EditorTransformComponentSelectionRequests::Pivot::Object;
        }
    }

    // take a generic, non-associative container and return a vector
    template<typename EntityIdContainer>
    static AZStd::vector<AZ::EntityId> EntityIdVectorFromContainer(const EntityIdContainer& entityIdContainer)
    {
        static_assert(AZStd::is_same<typename EntityIdContainer::value_type, AZ::EntityId>::value, "Container type is not an EntityId");

        AZ_PROFILE_FUNCTION(AzToolsFramework);
        return AZStd::vector<AZ::EntityId>(entityIdContainer.begin(), entityIdContainer.end());
    }

    // take a generic, associative container and return a vector
    template<typename EntityIdMap>
    static AZStd::vector<AZ::EntityId> EntityIdVectorFromMap(const EntityIdMap& entityIdMap)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZStd::vector<AZ::EntityId> entityIds;
        entityIds.reserve(entityIdMap.size());

        for (typename EntityIdMap::const_iterator it = entityIdMap.begin(); it != entityIdMap.end(); ++it)
        {
            entityIds.push_back(it->first);
        }

        return entityIds;
    }

    // WIP - currently just using positions - should use aabb here
    // e.g.
    //   get aabb
    //   get each edge/line of aabb
    //   convert to screen space
    //   test intersect segment against aabb screen rect

    template<typename EntitySelectFuncType, typename EntityIdContainer, typename Compare>
    static void BoxSelectAddRemoveToEntitySelection(
        const AZStd::optional<QRect>& boxSelect,
        const AzFramework::ScreenPoint& screenPosition,
        const AZ::EntityId visibleEntityId,
        const EntityIdContainer& incomingEntityIds,
        EntityIdContainer& outgoingEntityIds,
        EditorTransformComponentSelection& entityTransformComponentSelection,
        EntitySelectFuncType selectFunc1,
        EntitySelectFuncType selectFunc2,
        Compare outgoingCheck)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (boxSelect->contains(ViewportInteraction::QPointFromScreenPoint(screenPosition)))
        {
            const auto entityIt = incomingEntityIds.find(visibleEntityId);

            if (outgoingCheck(entityIt, incomingEntityIds))
            {
                outgoingEntityIds.insert(visibleEntityId);
                (entityTransformComponentSelection.*selectFunc1)(visibleEntityId);
            }
        }
        else
        {
            const auto entityIt = outgoingEntityIds.find(visibleEntityId);

            if (entityIt != outgoingEntityIds.end())
            {
                outgoingEntityIds.erase(entityIt);
                (entityTransformComponentSelection.*selectFunc2)(visibleEntityId);
            }
        }
    }

    template<typename EntityIdContainer>
    static void EntityBoxSelectUpdateGeneral(
        const AZStd::optional<QRect>& boxSelect,
        EditorTransformComponentSelection& editorTransformComponentSelection,
        const EntityIdContainer& activeSelectedEntityIds,
        EntityIdContainer& selectedEntityIdsBeforeBoxSelect,
        EntityIdContainer& potentialSelectedEntityIds,
        EntityIdContainer& potentialDeselectedEntityIds,
        const EditorVisibleEntityDataCacheInterface& entityDataCache,
        const int viewportId,
        const ViewportInteraction::KeyboardModifiers currentKeyboardModifiers,
        const ViewportInteraction::KeyboardModifiers& previousKeyboardModifiers)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (boxSelect)
        {
            // modifier has changed - swapped from additive to subtractive box select (or vice versa)
            if (previousKeyboardModifiers != currentKeyboardModifiers)
            {
                for (const AZ::EntityId& entityId : potentialDeselectedEntityIds)
                {
                    editorTransformComponentSelection.AddEntityToSelection(entityId);
                }

                potentialDeselectedEntityIds.clear();

                for (const AZ::EntityId& entityId : potentialSelectedEntityIds)
                {
                    editorTransformComponentSelection.RemoveEntityFromSelection(entityId);
                }

                potentialSelectedEntityIds.clear();
            }

            const AzFramework::CameraState cameraState = GetCameraState(viewportId);
            for (size_t entityCacheIndex = 0; entityCacheIndex < entityDataCache.VisibleEntityDataCount(); ++entityCacheIndex)
            {
                if (!entityDataCache.IsVisibleEntityIndividuallySelectableInViewport(entityCacheIndex))
                {
                    continue;
                }

                const AZ::EntityId entityId = entityDataCache.GetVisibleEntityId(entityCacheIndex);
                const AZ::Vector3& entityPosition = entityDataCache.GetVisibleEntityPosition(entityCacheIndex);

                const AzFramework::ScreenPoint screenPosition = AzFramework::WorldToScreen(entityPosition, cameraState);

                if (currentKeyboardModifiers.Ctrl())
                {
                    BoxSelectAddRemoveToEntitySelection(
                        boxSelect, screenPosition, entityId, selectedEntityIdsBeforeBoxSelect, potentialDeselectedEntityIds,
                        editorTransformComponentSelection, &EditorTransformComponentSelection::RemoveEntityFromSelection,
                        &EditorTransformComponentSelection::AddEntityToSelection,
                        [](const typename EntityIdContainer::const_iterator entityId, const EntityIdContainer& entityIds)
                        {
                            return entityId != entityIds.end();
                        });
                }
                else
                {
                    BoxSelectAddRemoveToEntitySelection(
                        boxSelect, screenPosition, entityId, activeSelectedEntityIds, potentialSelectedEntityIds,
                        editorTransformComponentSelection, &EditorTransformComponentSelection::AddEntityToSelection,
                        &EditorTransformComponentSelection::RemoveEntityFromSelection,
                        [](const typename EntityIdContainer::const_iterator entityId, const EntityIdContainer& entityIds)
                        {
                            return entityId == entityIds.end();
                        });
                }
            }
        }
    }

    static void InitializeTranslationLookup(EntityIdManipulators& entityIdManipulators)
    {
        for (auto& entityIdLookup : entityIdManipulators.m_lookups)
        {
            entityIdLookup.second.m_initial = AZ::Transform::CreateTranslation(GetWorldTranslation(entityIdLookup.first));
        }
    }

    static void DestroyCluster(const ViewportUi::ClusterId clusterId)
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, clusterId);
    }

    static void SetViewportUiClusterVisible(const ViewportUi::ClusterId clusterId, const bool visible)
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterVisible, clusterId, visible);
    }

    static void SetViewportUiSwitcherVisible(const ViewportUi::SwitcherId switcherId, const bool visible)
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetSwitcherVisible, switcherId, visible);
    }

    static void SetViewportUiClusterActiveButton(const ViewportUi::ClusterId clusterId, const ViewportUi::ButtonId buttonId)
    {
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, clusterId, buttonId);
    }

    static ViewportUi::ButtonId RegisterClusterButton(const ViewportUi::ClusterId clusterId, const char* iconName)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    void SnappingCluster::TrySetVisible(const bool visible)
    {
        bool snapping = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            snapping, ViewportUi::DefaultViewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::GridSnappingEnabled);

        // show snapping viewport ui only if there are entities selected and snapping is enabled
        SetViewportUiClusterVisible(m_clusterId, visible && snapping);
    }

    // return either center or entity pivot
    static AZ::Vector3 CalculatePivotTranslation(const AZ::EntityId entityId, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        return worldFromLocal.TransformPoint(CalculateCenterOffset(entityId, pivot));
    }

    void EditorTransformComponentSelection::SetAllViewportUiVisible(const bool visible)
    {
        SetViewportUiClusterVisible(m_transformModeClusterId, visible);
        SetViewportUiClusterVisible(m_spaceCluster.m_clusterId, visible);
        SetViewportUiClusterVisible(m_snappingCluster.m_clusterId, visible);
        m_viewportUiVisible = visible;
    }

    void EditorTransformComponentSelection::UpdateSpaceCluster(const ReferenceFrame referenceFrame)
    {
        auto buttonIdFromFrameFn = [this](const ReferenceFrame referenceFrame)
        {
            switch (referenceFrame)
            {
            case ReferenceFrame::Local:
                return m_spaceCluster.m_localButtonId;
            case ReferenceFrame::Parent:
                return m_spaceCluster.m_parentButtonId;
            case ReferenceFrame::World:
                return m_spaceCluster.m_worldButtonId;
            }
            return m_spaceCluster.m_parentButtonId;
        };

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_spaceCluster.m_clusterId,
            buttonIdFromFrameFn(referenceFrame));
    }

    namespace Etcs
    {
        PivotOrientationResult CalculatePivotOrientation(const AZ::EntityId entityId, const ReferenceFrame referenceFrame)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // initialize to world space, no parent
            PivotOrientationResult result{ AZ::Quaternion::CreateIdentity(), AZ::EntityId() };

            switch (referenceFrame)
            {
            case ReferenceFrame::Local:
                AZ::TransformBus::EventResult(result.m_worldOrientation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                break;
            case ReferenceFrame::Parent:
                {
                    AZ::EntityId parentId;
                    AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                    if (parentId.IsValid())
                    {
                        AZ::TransformBus::EventResult(
                            result.m_worldOrientation, parentId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);

                        result.m_parentId = parentId;
                    }
                }
                break;
            case ReferenceFrame::World:
                // noop (result is initialized to identity/world)
                break;
            }

            return result;
        }
    } // namespace Etcs

    // return parent space from selection - if entity(s) share a common parent,
    // return that space, otherwise return world space
    template<typename EntityIdMapIterator>
    static Etcs::PivotOrientationResult CalculateParentSpace(EntityIdMapIterator begin, EntityIdMapIterator end)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // initialize to world with no parent
        Etcs::PivotOrientationResult result{ AZ::Quaternion::CreateIdentity(), AZ::EntityId() };

        AZ::EntityId commonParentId;
        for (EntityIdMapIterator entityIdLookupIt = begin; entityIdLookupIt != end; ++entityIdLookupIt)
        {
            // check if this entity has a parent
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, entityIdLookupIt->first, &AZ::TransformBus::Events::GetParentId);

            // if no parent, space will be world, terminate
            if (!parentId.IsValid())
            {
                commonParentId.SetInvalid();
                result.m_worldOrientation = AZ::Quaternion::CreateIdentity();

                break;
            }

            // if we've not yet set common parent, set it to first valid parent id
            if (!commonParentId.IsValid())
            {
                commonParentId = parentId;
                AZ::TransformBus::EventResult(result.m_worldOrientation, parentId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
            }

            // if we know we still have a parent in common
            if (parentId != commonParentId)
            {
                // if the current parent is different to the common parent,
                // space will be world, terminate
                commonParentId.SetInvalid();
                result.m_worldOrientation = AZ::Quaternion::CreateIdentity();

                break;
            }
        }

        // finally record if we have a parent or not
        result.m_parentId = commonParentId;

        return result;
    }

    template<typename EntityIdMap>
    static AZ::Vector3 CalculatePivotTranslationForEntityIds(
        const EntityIdMap& entityIdMap, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!entityIdMap.empty())
        {
            // calculate average position of selected vertices for translation manipulator
            AZ::Vector3 minPosition = AZ::Vector3(AZ::Constants::FloatMax);
            AZ::Vector3 maxPosition = AZ::Vector3(-AZ::Constants::FloatMax);
            for (const auto& entityId : entityIdMap)
            {
                const AZ::Vector3 pivotPosition = CalculatePivotTranslation(entityId.first, pivot);
                minPosition = pivotPosition.GetMin(minPosition);
                maxPosition = pivotPosition.GetMax(maxPosition);
            }

            return minPosition + (maxPosition - minPosition) * 0.5f;
        }

        return AZ::Vector3::CreateZero();
    }

    namespace Etcs
    {
        template<typename EntityIdMap>
        PivotOrientationResult CalculatePivotOrientationForEntityIds(const EntityIdMap& entityIdMap, const ReferenceFrame referenceFrame)
        {
            static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // simple case with one entity
            if (entityIdMap.size() == 1)
            {
                return Etcs::CalculatePivotOrientation(entityIdMap.begin()->first, referenceFrame);
            }

            // local/parent logic the same
            switch (referenceFrame)
            {
            case ReferenceFrame::Local:
            case ReferenceFrame::Parent:
                return CalculateParentSpace(entityIdMap.begin(), entityIdMap.end());
            case ReferenceFrame::World:
                [[fallthrough]];
            default:
                return { AZ::Quaternion::CreateIdentity(), AZ::EntityId() };
            }
        }
    } // namespace Etcs

    namespace Etcs
    {
        template<typename EntityIdMap>
        PivotOrientationResult CalculateSelectionPivotOrientation(
            const EntityIdMap& entityIdMap, const OptionalFrame& pivotOverrideFrame, const ReferenceFrame referenceFrame)
        {
            static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");
            static_assert(
                AZStd::is_same<typename EntityIdMap::mapped_type, EntityIdManipulatorLookup>::value,
                "Container value type is not an EntityIdManipulators::Lookup");

            AZ_PROFILE_FUNCTION(AzToolsFramework);

            // start - calculate orientation without considering current overrides/modifications
            PivotOrientationResult pivot = CalculatePivotOrientationForEntityIds(entityIdMap, referenceFrame);

            // if there is already an orientation override
            if (pivotOverrideFrame.HasOrientationOverride())
            {
                switch (referenceFrame)
                {
                case ReferenceFrame::Local:
                case ReferenceFrame::Parent:
                    pivot.m_worldOrientation = pivotOverrideFrame.m_orientationOverride.value();
                    break;
                case ReferenceFrame::World:
                    pivot.m_worldOrientation = AZ::Quaternion::CreateIdentity();
                    break;
                }
            }
            else
            {
                if (entityIdMap.size() > 1)
                {
                    // if there's no orientation override and reference frame is parent, orientation
                    // should be aligned to world frame/space when there's more than one entity
                    // (unless all selected entities share a common parent)
                    switch (referenceFrame)
                    {
                    case ReferenceFrame::Parent:
                        if (pivot.m_parentId.IsValid())
                        {
                            break;
                        }
                        [[fallthrough]];
                    case ReferenceFrame::World:
                        pivot.m_worldOrientation = AZ::Quaternion::CreateIdentity();
                        break;
                    }
                }
            }

            return pivot;
        }
    } // namespace Etcs

    template<typename EntityIdMap>
    static AZ::Vector3 RecalculateAverageManipulatorTranslation(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        return pivotOverrideFrame.m_translationOverride.value_or(CalculatePivotTranslationForEntityIds(entityIdMap, pivot));
    }

    template<typename EntityIdMap>
    static AZ::Quaternion RecalculateAverageManipulatorOrientation(
        const EntityIdMap& entityIdMap, const OptionalFrame& pivotOverrideFrame, const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        return Etcs::CalculateSelectionPivotOrientation(entityIdMap, pivotOverrideFrame, referenceFrame).m_worldOrientation;
    }

    template<typename EntityIdMap>
    static AZ::Transform RecalculateAverageManipulatorTransform(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot,
        const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // return final transform, if we have an override for translation use that, otherwise
        // use centered translation of selection
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            RecalculateAverageManipulatorOrientation(entityIdMap, pivotOverrideFrame, referenceFrame),
            RecalculateAverageManipulatorTranslation(entityIdMap, pivotOverrideFrame, pivot));
    }

    template<typename EntityIdMap>
    static void BuildSortedEntityIdVectorFromEntityIdMap(const EntityIdMap& entityIds, EntityIdList& sortedEntityIdsOut)
    {
        sortedEntityIdsOut = EntityIdVectorFromMap(entityIds);
        SortEntitiesByLocationInHierarchy(sortedEntityIdsOut);
    }

    static void UpdateInitialTransform(EntityIdManipulators& entityManipulators)
    {
        // save new start orientation (if moving rotation axes separate from object
        // or switching type of rotation (modifier keys change))
        for (auto& entityIdLookup : entityManipulators.m_lookups)
        {
            entityIdLookup.second.m_initial = GetWorldTransform(entityIdLookup.first);
        }
    }

    // utility function to immediately return the current reference frame based on the state of the modifiers
    static ReferenceFrame ReferenceFrameFromModifiers(const ViewportInteraction::KeyboardModifiers modifiers)
    {
        return modifiers.Shift() ? ReferenceFrame::World : ReferenceFrame::Local;
    }

    // utility function to immediately return the current sphere of influence of the manipulators based on the
    // state of the modifiers
    static Influence InfluenceFromModifiers(const ViewportInteraction::KeyboardModifiers modifiers)
    {
        return modifiers.Alt() ? Influence::Individual : Influence::Group;
    }

    template<typename Action, typename EntityIdContainer>
    static void UpdateTranslationManipulator(
        const Action& action,
        const EntityIdContainer& entityIdContainer,
        EntityIdManipulators& entityIdManipulators,
        OptionalFrame& pivotOverrideFrame,
        ViewportInteraction::KeyboardModifiers& prevModifiers,
        bool& transformChangedInternally,
        const AZStd::optional<ReferenceFrame> spaceLock)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        entityIdManipulators.m_manipulators->SetLocalPosition(action.LocalPosition());

        if (action.m_modifiers.Ctrl())
        {
            // moving with ctrl - setting override
            pivotOverrideFrame.m_translationOverride = entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            InitializeTranslationLookup(entityIdManipulators);
        }
        else
        {
            const ReferenceFrame referenceFrame = spaceLock.value_or(ReferenceFrameFromModifiers(action.m_modifiers));
            const Influence influence = InfluenceFromModifiers(action.m_modifiers);

            // note: used for parent and world depending on the current reference frame
            const auto pivotOrientation =
                Etcs::CalculateSelectionPivotOrientation(entityIdManipulators.m_lookups, pivotOverrideFrame, referenceFrame);

            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (AZ::EntityId entityId : entityIdContainer)
            {
                const auto entityItLookupIt = entityIdManipulators.m_lookups.find(entityId);
                if (entityItLookupIt == entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Vector3 worldTranslation = GetWorldTranslation(entityId);

                switch (influence)
                {
                case Influence::Individual:
                    {
                        // move in each entities local space at once
                        AZ::Quaternion worldOrientation = AZ::Quaternion::CreateIdentity();
                        AZ::TransformBus::EventResult(worldOrientation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);

                        const AZ::Transform space = entityIdManipulators.m_manipulators->GetLocalTransform().GetInverse() *
                            AZ::Transform::CreateFromQuaternionAndTranslation(worldOrientation, worldTranslation);

                        const AZ::Vector3 localOffset = space.TransformVector(action.LocalPositionOffset());

                        if (action.m_modifiers != prevModifiers)
                        {
                            entityItLookupIt->second.m_initial = AZ::Transform::CreateTranslation(worldTranslation - localOffset);
                        }

                        Etcs::SetEntityWorldTranslation(
                            entityId, entityItLookupIt->second.m_initial.GetTranslation() + localOffset, transformChangedInternally);
                    }
                    break;
                case Influence::Group:
                    {
                        const AZ::Quaternion offsetRotation = pivotOrientation.m_worldOrientation *
                            QuaternionFromTransformNoScaling(entityIdManipulators.m_manipulators->GetLocalTransform().GetInverse());

                        const AZ::Vector3 localOffset = offsetRotation.TransformVector(action.LocalPositionOffset());

                        if (action.m_modifiers != prevModifiers)
                        {
                            entityItLookupIt->second.m_initial = AZ::Transform::CreateTranslation(worldTranslation - localOffset);
                        }

                        Etcs::SetEntityWorldTranslation(
                            entityId, entityItLookupIt->second.m_initial.GetTranslation() + localOffset, transformChangedInternally);
                    }
                    break;
                }
            }

            // if transform pivot override has been set, make sure to update it when we move it
            if (pivotOverrideFrame.m_translationOverride)
            {
                pivotOverrideFrame.m_translationOverride = entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            }
        }

        prevModifiers = action.m_modifiers;
    }

    void HandleAccents(
        const AZ::EntityId currentEntityIdUnderCursor,
        AZ::EntityId& hoveredEntityIdUnderCursor,
        const HandleAccentsContext& handleAccentsContext,
        const ViewportInteraction::MouseButtons mouseButtons,
        const AZStd::function<void(AZ::EntityId, bool)>& setEntityAccentedFn)
    {
        const bool invalidMouseButtonHeld = mouseButtons.Middle() || mouseButtons.Right();
        const bool hasSelectedEntities = handleAccentsContext.m_hasSelectedEntities;
        const bool ctrlHeld = handleAccentsContext.m_ctrlHeld;
        const bool boxSelect = handleAccentsContext.m_usingBoxSelect;
        const bool stickySelect = handleAccentsContext.m_usingStickySelect;
        const bool canSelect = stickySelect ? !hasSelectedEntities || ctrlHeld : true;

        const bool removePreviousAccent =
            (currentEntityIdUnderCursor != hoveredEntityIdUnderCursor && hoveredEntityIdUnderCursor.IsValid()) || invalidMouseButtonHeld;
        const bool addNextAccent = currentEntityIdUnderCursor.IsValid() && canSelect && !invalidMouseButtonHeld && !boxSelect;

        if (removePreviousAccent)
        {
            setEntityAccentedFn(hoveredEntityIdUnderCursor, false);
            hoveredEntityIdUnderCursor.SetInvalid();
        }

        if (addNextAccent)
        {
            setEntityAccentedFn(currentEntityIdUnderCursor, true);
            hoveredEntityIdUnderCursor = currentEntityIdUnderCursor;
        }
    }

    // is the passed entity id contained with in the entity id list
    template<typename EntityIdContainer>
    static bool IsEntitySelectedInternal(AZ::EntityId entityId, const EntityIdContainer& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        const auto entityIdIt = selectedEntityIds.find(entityId);
        return entityIdIt != selectedEntityIds.end();
    }

    static EntityIdTransformMap RecordTransformsBefore(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // save initial transforms - this is necessary in cases where entities exist
        // in a hierarchy. We want to make sure a parent transform does not affect
        // a child transform (entities seeming to get the same transform applied twice)
        EntityIdTransformMap transformsBefore;
        transformsBefore.reserve(entityIds.size());
        for (AZ::EntityId entityId : entityIds)
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            transformsBefore.insert({ entityId, worldFromLocal });
        }

        return transformsBefore;
    }

    // ask the visible entity data cache if the entity is selectable in the viewport
    // (useful in the context of drawing when we only care about entities we can see)
    // note: return the index if it is selectable, nullopt otherwise
    static AZStd::optional<size_t> SelectableInVisibleViewportCache(
        const EditorVisibleEntityDataCacheInterface& entityDataCache, const AZ::EntityId entityId)
    {
        if (auto entityIndex = entityDataCache.GetVisibleEntityIndexFromId(entityId))
        {
            if (entityDataCache.IsVisibleEntityIndividuallySelectableInViewport(*entityIndex))
            {
                return *entityIndex;
            }
        }

        return AZStd::nullopt;
    }

    static AZ::ComponentId GetTransformComponentId(const AZ::EntityId entityId)
    {
        if (const AZ::Entity* entity = GetEntityById(entityId))
        {
            if (const AZ::Component* component = entity->FindComponent<Components::TransformComponent>())
            {
                return component->GetId();
            }
        }

        return AZ::InvalidComponentId;
    }

    // must be called after a property is changed on an entity component property
    // note: it is not required if the property was modified directly by a manipulator as this logic
    // is handled internally - this call is often required after an action/shortcut of some kind
    static void RefreshUiAfterChange(const EntityIdList& entitiyIds)
    {
        EditorTransformChangeNotificationBus::Broadcast(&EditorTransformChangeNotifications::OnEntityTransformChanged, entitiyIds);
        ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
    }

    static AZ::Vector3 EtcsPickEntity(const AZ::EntityId entityId, const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        float distance;
        const auto& origin = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayOrigin;
        const auto& direction = mouseInteraction.m_mouseInteraction.m_mousePick.m_rayDirection;
        if (AzToolsFramework::PickEntity(
                entityId, origin, direction, distance, mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId))
        {
            return origin + direction * distance;
        }

        return origin + direction * GetDefaultEntityPlacementDistance();
    }

    EditorTransformComponentSelection::EditorTransformComponentSelection(const EditorVisibleEntityDataCacheInterface* entityDataCache)
        : m_entityDataCache(entityDataCache)
    {
        const AzFramework::EntityContextId entityContextId = GetEntityContextId();

        m_editorHelpers = AZStd::make_unique<EditorHelpers>(entityDataCache);

        ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusConnect(entityContextId);
        ToolsApplicationNotificationBus::Handler::BusConnect();
        Camera::EditorCameraNotificationBus::Handler::BusConnect();
        ViewportEditorModeNotificationsBus::Handler::BusConnect(entityContextId);
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusConnect(entityContextId);
        ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusConnect(ViewportUi::DefaultViewportId);
        ReadOnlyEntityPublicNotificationBus::Handler::BusConnect(entityContextId);

        CreateComponentModeSwitcher();
        CreateTransformModeSelectionCluster();
        CreateSpaceSelectionCluster();
        CreateSnappingCluster();

        m_actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(
            m_actionManagerInterface, "PrefabCould not get ActionManagerInterface on EditorTransformComponentSelection construction.");

        m_hotKeyManagerInterface = AZ::Interface<HotKeyManagerInterface>::Get();
        AZ_Assert(
            m_hotKeyManagerInterface, "PrefabCould not get HotKeyManagerInterface on EditorTransformComponentSelection construction.");

        m_menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(
            m_menuManagerInterface, "PrefabCould not get MenuManagerInterface on EditorTransformComponentSelection construction.");

        SetupBoxSelect();
        RefreshSelectedEntityIdsAndRegenerateManipulators();

        // ensure the click detector uses the EditorViewportInputTimeNowRequests interface to retrieve elapsed time
        // note: this is to facilitate overriding this functionality for purposes such as testing
        m_clickDetector.OverrideTimeNowFn(
            []
            {
                AZStd::chrono::milliseconds timeNow;
                AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::BroadcastResult(
                    timeNow,
                    &AzToolsFramework::ViewportInteraction::EditorViewportInputTimeNowRequestBus::Events::EditorViewportInputTimeNow);
                return timeNow;
            }
        );

        EditorTransformComponentSelectionInternal::SetCurrentInstance(this);
    }

    EditorTransformComponentSelection::~EditorTransformComponentSelection()
    {
        m_selectedEntityIds.clear();
        DestroyManipulators(m_entityIdManipulators);

        DestroyCluster(m_transformModeClusterId);
        DestroyCluster(m_spaceCluster.m_clusterId);
        DestroyCluster(m_snappingCluster.m_clusterId);

        m_pivotOverrideFrame.Reset();

        ReadOnlyEntityPublicNotificationBus::Handler::BusDisconnect();
        ViewportInteraction::ViewportSettingsNotificationBus::Handler::BusDisconnect();
        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusDisconnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterDisconnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        Camera::EditorCameraNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusDisconnect();
        ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

        EditorTransformComponentSelectionInternal::SetCurrentInstance(nullptr);
    }

    void EditorTransformComponentSelection::SetupBoxSelect()
    {
        // data to be used during a box select action
        struct EntityBoxSelectData
        {
            AZStd::unique_ptr<SelectionCommand> m_boxSelectSelectionCommand; // track all added/removed entities after a BoxSelect
            AZStd::unordered_set<AZ::EntityId> m_selectedEntityIdsBeforeBoxSelect;
            AZStd::unordered_set<AZ::EntityId> m_potentialSelectedEntityIds;
            AZStd::unordered_set<AZ::EntityId> m_potentialDeselectedEntityIds;
        };

        // lambdas capture shared_ptr by value to increment ref count
        auto entityBoxSelectData = AZStd::make_shared<EntityBoxSelectData>();

        m_boxSelect.InstallLeftMouseDown(
            [this, entityBoxSelectData]([[maybe_unused]] const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
            {
                // begin selection undo/redo command
                entityBoxSelectData->m_boxSelectSelectionCommand =
                    AZStd::make_unique<SelectionCommand>(EntityIdList(), EntityBoxSelectUndoRedoDesc);
                // grab currently selected entities
                entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect = m_selectedEntityIds;
            });

        m_boxSelect.InstallMouseMove(
            [this, entityBoxSelectData](const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
            {
                EntityBoxSelectUpdateGeneral(
                    m_boxSelect.BoxRegion(), *this, m_selectedEntityIds, entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect,
                    entityBoxSelectData->m_potentialSelectedEntityIds, entityBoxSelectData->m_potentialDeselectedEntityIds,
                    *m_entityDataCache, mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                    mouseInteraction.m_mouseInteraction.m_keyboardModifiers, m_boxSelect.PreviousModifiers());
            });

        m_boxSelect.InstallLeftMouseUp(
            [this, entityBoxSelectData]
            {
                entityBoxSelectData->m_boxSelectSelectionCommand->UpdateSelection(EntityIdVectorFromContainer(m_selectedEntityIds));

                // if we know a change in selection has occurred, record the undo step
                if (!entityBoxSelectData->m_potentialDeselectedEntityIds.empty() ||
                    !entityBoxSelectData->m_potentialSelectedEntityIds.empty())
                {
                    ScopedUndoBatch undoBatch(EntityBoxSelectUndoRedoDesc);

                    // restore manipulator overrides when undoing
                    if (m_entityIdManipulators.m_manipulators && m_selectedEntityIds.empty())
                    {
                        CreateEntityManipulatorDeselectCommand(undoBatch);
                    }

                    entityBoxSelectData->m_boxSelectSelectionCommand->SetParent(undoBatch.GetUndoBatch());
                    entityBoxSelectData->m_boxSelectSelectionCommand.release();

                    SetSelectedEntities(EntityIdVectorFromContainer(m_selectedEntityIds));
                    // note: manipulators will be updated in AfterEntitySelectionChanged

                    // clear pivot override when selection is empty
                    if (m_selectedEntityIds.empty())
                    {
                        m_pivotOverrideFrame.Reset();
                    }
                }
                else
                {
                    entityBoxSelectData->m_boxSelectSelectionCommand.reset();
                }

                entityBoxSelectData->m_potentialSelectedEntityIds.clear();
                entityBoxSelectData->m_potentialDeselectedEntityIds.clear();
                entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.clear();
            });

        m_boxSelect.InstallDisplayScene(
            [this, entityBoxSelectData](const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
            {
                if (const auto keyboardModifiers = AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();
                    m_boxSelect.PreviousModifiers() != keyboardModifiers)
                {
                    EntityBoxSelectUpdateGeneral(
                        m_boxSelect.BoxRegion(), *this, m_selectedEntityIds, entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect,
                        entityBoxSelectData->m_potentialSelectedEntityIds, entityBoxSelectData->m_potentialDeselectedEntityIds,
                        *m_entityDataCache, viewportInfo.m_viewportId, keyboardModifiers, m_boxSelect.PreviousModifiers());
                }

                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(SelectedEntityAabbColor);

                for (AZ::EntityId entityId : entityBoxSelectData->m_potentialSelectedEntityIds)
                {
                    const auto entityIdIt = entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.find(entityId);

                    // don't show box when re-adding from previous selection
                    if (entityIdIt != entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.end())
                    {
                        continue;
                    }

                    if (const AZ::Aabb bound = CalculateEditorEntitySelectionBounds(entityId, viewportInfo); bound.IsValid())
                    {
                        debugDisplay.DrawSolidBox(bound.GetMin(), bound.GetMax());
                    }
                }

                debugDisplay.DepthTestOn();
            });
    }

    EntityManipulatorCommand::State EditorTransformComponentSelection::CreateManipulatorCommandStateFromSelf() const
    {
        // if we're in the process of creating a new entity without a selection we
        // will not have created any manipulators at this point - to record this state
        // just return a default constructed EntityManipulatorCommand which will not be used
        if (!m_entityIdManipulators.m_manipulators)
        {
            return {};
        }

        return { BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                 TransformNormalizedScale(m_entityIdManipulators.m_manipulators->GetLocalTransform()) };
    }

    void EditorTransformComponentSelection::BeginRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // we must have an existing parent undo batch active when beginning to record
        // a manipulator command
        UndoSystem::URSequencePoint* currentUndoOperation = nullptr;
        ToolsApplicationRequests::Bus::BroadcastResult(currentUndoOperation, &ToolsApplicationRequests::GetCurrentUndoBatch);

        if (currentUndoOperation)
        {
            // check here if translation or orientation override are set
            m_manipulatorMoveCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);
        }
    }

    void EditorTransformComponentSelection::EndRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_manipulatorMoveCommand)
        {
            m_manipulatorMoveCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            UndoSystem::URSequencePoint* currentUndoOperation = nullptr;
            ToolsApplicationRequests::Bus::BroadcastResult(currentUndoOperation, &ToolsApplicationRequests::GetCurrentUndoBatch);

            AZ_Assert(
                currentUndoOperation,
                "The only way we should have reached this block is if "
                "m_manipulatorMoveCommand was created by calling BeginRecordManipulatorMouseMoveCommand. "
                "If we've reached this point and currentUndoOperation is null, something bad has happened "
                "in the undo system");

            if (currentUndoOperation)
            {
                m_manipulatorMoveCommand->SetParent(currentUndoOperation);
                m_manipulatorMoveCommand.release();
            }
        }
    }

    void EditorTransformComponentSelection::CreateTranslationManipulators()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZStd::unique_ptr<TranslationManipulators> translationManipulators = AZStd::make_unique<TranslationManipulators>(
            TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne());
        translationManipulators->SetLineBoundWidth(ManipulatorLineBoundWidth());

        InitializeManipulators(*translationManipulators);

        ConfigureTranslationManipulatorAppearance3d(&*translationManipulators);

        translationManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        // lambdas capture shared_ptr by value to increment ref count
        auto manipulatorEntityIds = AZStd::make_shared<ManipulatorEntityIds>();

        // [ref 1.] Note: in  BaseManipulator, for every mouse down event, we automatically start
        // recording an undo command in case something changes - here we take advantage of that
        // knowledge by asking for the current undo operation to add our manipulator entity command to

        // linear
        translationManipulators->InstallLinearManipulatorMouseDownCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const LinearManipulator::Action& action) mutable
            {
                // important to sort entityIds based on hierarchy order when updating transforms
                BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

                InitializeTranslationLookup(m_entityIdManipulators);

                m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
                m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

                // see comment [ref 1.] above
                BeginRecordManipulatorCommand();
            });

        ViewportInteraction::KeyboardModifiers prevModifiers{};
        translationManipulators->InstallLinearManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const LinearManipulator::Action& action) mutable
            {
                UpdateTranslationManipulator(
                    action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators, m_pivotOverrideFrame, prevModifiers,
                    m_transformChangedInternally, m_spaceCluster.m_spaceLock);
            });

        translationManipulators->InstallLinearManipulatorMouseUpCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const LinearManipulator::Action& action) mutable
            {
                AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                    &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                    manipulatorEntityIds->m_entityIds);

                EndRecordManipulatorCommand();
            });

        // planar
        translationManipulators->InstallPlanarManipulatorMouseDownCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const PlanarManipulator::Action& action)
            {
                // important to sort entityIds based on hierarchy order when updating transforms
                BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

                InitializeTranslationLookup(m_entityIdManipulators);

                m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
                m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

                // see comment [ref 1.] above
                BeginRecordManipulatorCommand();
            });

        translationManipulators->InstallPlanarManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const PlanarManipulator::Action& action) mutable
            {
                UpdateTranslationManipulator(
                    action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators, m_pivotOverrideFrame, prevModifiers,
                    m_transformChangedInternally, m_spaceCluster.m_spaceLock);
            });

        translationManipulators->InstallPlanarManipulatorMouseUpCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const PlanarManipulator::Action& action)
            {
                AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                    &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                    manipulatorEntityIds->m_entityIds);

                EndRecordManipulatorCommand();
            });

        // surface
        translationManipulators->InstallSurfaceManipulatorMouseDownCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const SurfaceManipulator::Action& action)
            {
                BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

                InitializeTranslationLookup(m_entityIdManipulators);

                m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
                m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

                // see comment [ref 1.] above
                BeginRecordManipulatorCommand();
            });

        translationManipulators->InstallSurfaceManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const SurfaceManipulator::Action& action) mutable
            {
                UpdateTranslationManipulator(
                    action, manipulatorEntityIds->m_entityIds, m_entityIdManipulators, m_pivotOverrideFrame, prevModifiers,
                    m_transformChangedInternally, m_spaceCluster.m_spaceLock);
            });

        translationManipulators->InstallSurfaceManipulatorMouseUpCallback(
            [this, manipulatorEntityIds]([[maybe_unused]] const SurfaceManipulator::Action& action)
            {
                AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                    &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                    manipulatorEntityIds->m_entityIds);

                EndRecordManipulatorCommand();
            });

        translationManipulators->InstallSurfaceManipulatorEntityIdsToIgnoreFn(
            [this](const ViewportInteraction::MouseInteraction& interaction)
            {
                if (interaction.m_keyboardModifiers.Ctrl())
                {
                    return AZStd::unordered_set<AZ::EntityId>();
                }
                else
                {
                    return m_selectedEntityIds;
                }
            });

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(translationManipulators);
    }

    void EditorTransformComponentSelection::CreateRotationManipulators()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZStd::unique_ptr<RotationManipulators> rotationManipulators =
            AZStd::make_unique<RotationManipulators>(AZ::Transform::CreateIdentity());
        rotationManipulators->SetCircleBoundWidth(ManipulatorCicleBoundWidth());

        InitializeManipulators(*rotationManipulators);

        rotationManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        // view
        rotationManipulators->SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            RotationManipulatorRadius(), AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        struct SharedRotationState
        {
            AZ::Quaternion m_savedOrientation = AZ::Quaternion::CreateIdentity();
            ReferenceFrame m_referenceFrameAtMouseDown = ReferenceFrame::Local;
            EntityIdList m_entityIds;
        };

        // lambdas capture shared_ptr by value to increment ref count
        AZStd::shared_ptr<SharedRotationState> sharedRotationState = AZStd::make_shared<SharedRotationState>();

        rotationManipulators->InstallLeftMouseDownCallback(
            [this, sharedRotationState](
                [[maybe_unused]] const AngularManipulator::Action& action) mutable
            {
                sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();
                sharedRotationState->m_referenceFrameAtMouseDown = m_referenceFrame;
                // important to sort entityIds based on hierarchy order when updating transforms
                BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, sharedRotationState->m_entityIds);

                UpdateInitialTransform(m_entityIdManipulators);

                m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
                m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

                // see comment [ref 1.] above
                BeginRecordManipulatorCommand();
            });

        rotationManipulators->InstallMouseMoveCallback(
            [this, prevModifiers = ViewportInteraction::KeyboardModifiers(), sharedRotationState](const AngularManipulator::Action& action) mutable
            {
                const ReferenceFrame referenceFrame = m_spaceCluster.m_spaceLock.value_or(ReferenceFrameFromModifiers(action.m_modifiers));
                const Influence influence = InfluenceFromModifiers(action.m_modifiers);

                const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;
                // store the pivot override frame when positioning the manipulator manually (ctrl)
                // so we don't lose the orientation when adding/removing entities from the selection
                if (action.m_modifiers.Ctrl())
                {
                    m_pivotOverrideFrame.m_orientationOverride = manipulatorOrientation;
                }

                // only update the manipulator orientation if we're rotating in a local reference frame or we're
                // manually modifying the manipulator orientation independent of the entity by holding ctrl
                if (sharedRotationState->m_referenceFrameAtMouseDown == ReferenceFrame::Local || action.m_modifiers.Ctrl())
                {
                    m_entityIdManipulators.m_manipulators->SetLocalTransform(AZ::Transform::CreateFromQuaternionAndTranslation(
                        manipulatorOrientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));
                }

                // save state if we change the type of rotation we're doing to to prevent snapping
                if (prevModifiers != action.m_modifiers)
                {
                    UpdateInitialTransform(m_entityIdManipulators);
                    sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
                }

                // allow the user to modify the orientation without moving the object if ctrl is held
                if (action.m_modifiers.Ctrl())
                {
                    UpdateInitialTransform(m_entityIdManipulators);
                    sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
                }
                else
                {
                    // only update the pivot override if the orientation is being modified in local space and we have
                    // more than one entity selected (so rotating a single entity does not set the orientation override)
                    if (referenceFrame == ReferenceFrame::Local && sharedRotationState->m_entityIds.size() > 1)
                    {
                        m_pivotOverrideFrame.m_orientationOverride = manipulatorOrientation;
                    }

                    // note: must use sorted entityIds based on hierarchy order when updating transforms
                    for (AZ::EntityId entityId : sharedRotationState->m_entityIds)
                    {
                        auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                        if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                        {
                            continue;
                        }

                        // make sure we take into account how we move the axis independent of object
                        // if Ctrl was held to adjust the orientation of the axes separately
                        const AZ::Transform offsetRotation =
                            AZ::Transform::CreateFromQuaternion(sharedRotationState->m_savedOrientation * action.m_current.m_delta);

                        switch (influence)
                        {
                        case Influence::Individual:
                            {
                                const AZ::Quaternion rotation = entityIdLookupIt->second.m_initial.GetRotation().GetNormalized();
                                const AZ::Vector3 position = entityIdLookupIt->second.m_initial.GetTranslation();
                                const float scale = entityIdLookupIt->second.m_initial.GetUniformScale();

                                const AZ::Vector3 centerOffset = CalculateCenterOffset(entityId, m_pivotMode);

                                // scale -> rotate -> translate
                                SetEntityWorldTransform(
                                    entityId,
                                    AZ::Transform::CreateTranslation(position) * AZ::Transform::CreateFromQuaternion(rotation) *
                                        AZ::Transform::CreateTranslation(centerOffset) * offsetRotation *
                                        AZ::Transform::CreateTranslation(-centerOffset) * AZ::Transform::CreateUniformScale(scale));
                            }
                            break;
                        case Influence::Group:
                            {
                                const AZ::Transform pivotTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                                    manipulatorOrientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());
                                const AZ::Transform transformInPivotSpace =
                                    pivotTransform.GetInverse() * entityIdLookupIt->second.m_initial;

                                SetEntityWorldTransform(entityId, pivotTransform * offsetRotation * transformInPivotSpace);
                            }
                            break;
                        }
                    }
                }

                prevModifiers = action.m_modifiers;
            });

        rotationManipulators->InstallLeftMouseUpCallback(
            [this, sharedRotationState]([[maybe_unused]] const AngularManipulator::Action& action)
            {
                AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                    &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                    sharedRotationState->m_entityIds);

                EndRecordManipulatorCommand();
            });

        rotationManipulators->Register(g_mainManipulatorManagerId);

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(rotationManipulators);
    }

    void EditorTransformComponentSelection::CreateScaleManipulators()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZStd::unique_ptr<ScaleManipulators> scaleManipulators = AZStd::make_unique<ScaleManipulators>(AZ::Transform::CreateIdentity());
        scaleManipulators->SetLineBoundWidth(ManipulatorLineBoundWidth());

        InitializeManipulators(*scaleManipulators);

        scaleManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        scaleManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        scaleManipulators->ConfigureView(
            LinearManipulatorAxisLength(), AZ::Color::CreateOne(), AZ::Color::CreateOne(), AZ::Color::CreateOne());

        struct SharedScaleState
        {
            AZ::Vector3 m_savedScaleOffset = AZ::Vector3::CreateZero();
            float m_initialViewScale = 0.0f;
            EntityIdList m_entityIds;
        };

        // lambdas capture shared_ptr by value to increment ref count
        auto sharedScaleState = AZStd::make_shared<SharedScaleState>();

        auto uniformLeftMouseDownCallback = [this, sharedScaleState]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            sharedScaleState->m_savedScaleOffset = AZ::Vector3::CreateZero();
            sharedScaleState->m_initialViewScale = ManipulatorViewBaseScale();
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, sharedScaleState->m_entityIds);

            UpdateInitialTransform(m_entityIdManipulators);

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());
        };

        auto uniformLeftMouseUpCallback = [this, sharedScaleState]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged, sharedScaleState->m_entityIds);

            m_entityIdManipulators.m_manipulators->SetLocalTransform(RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));
        };

        auto uniformLeftMouseMoveCallback = [this, sharedScaleState, prevModifiers = ViewportInteraction::KeyboardModifiers()](
                                                const LinearManipulator::Action& action) mutable
        {
            if (prevModifiers != action.m_modifiers)
            {
                UpdateInitialTransform(m_entityIdManipulators);
                sharedScaleState->m_savedScaleOffset = action.LocalScaleOffset();
                sharedScaleState->m_initialViewScale = ManipulatorViewBaseScale();
            }

            const auto sumVectorElements = [](const AZ::Vector3& vec)
            {
                return vec.GetX() + vec.GetY() + vec.GetZ();
            };

            const float uniformScale =
                action.m_start.m_sign * sumVectorElements(action.LocalScaleOffset() - sharedScaleState->m_savedScaleOffset);

            if (action.m_modifiers.Ctrl())
            {
                // adjust the manipulator scale when ctrl is held
                SetManipulatorViewBaseScale(AZ::GetClamp(
                    sharedScaleState->m_initialViewScale + uniformScale, MinManipulatorViewBaseScale, MaxManipulatorViewBaseScale));
            }
            else
            {
                // note: must use sorted entityIds based on hierarchy order when updating transforms
                for (AZ::EntityId entityId : sharedScaleState->m_entityIds)
                {
                    auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                    if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                    {
                        continue;
                    }

                    const AZ::Transform initial = entityIdLookupIt->second.m_initial;
                    const float initialScale = initial.GetUniformScale();

                    const float scale = AZ::GetClamp(1.0f + uniformScale / initialScale, AZ::MinTransformScale, AZ::MaxTransformScale);
                    const AZ::Transform scaleTransform = AZ::Transform::CreateUniformScale(scale);

                    switch (InfluenceFromModifiers(action.m_modifiers))
                    {
                    case Influence::Individual:
                        {
                            const AZ::Transform pivotTransform = TransformNormalizedScale(entityIdLookupIt->second.m_initial);
                            const AZ::Transform transformInPivotSpace = pivotTransform.GetInverse() * initial;

                            SetEntityWorldTransform(entityId, pivotTransform * scaleTransform * transformInPivotSpace);
                        }
                        break;
                    case Influence::Group:
                        {
                            const AZ::Transform pivotTransform =
                                TransformNormalizedScale(m_entityIdManipulators.m_manipulators->GetLocalTransform());
                            const AZ::Transform transformInPivotSpace = pivotTransform.GetInverse() * initial;

                            SetEntityWorldTransform(entityId, pivotTransform * scaleTransform * transformInPivotSpace);
                        }
                    }
                }
            }

            prevModifiers = action.m_modifiers;
        };

        scaleManipulators->InstallAxisLeftMouseDownCallback(uniformLeftMouseDownCallback);
        scaleManipulators->InstallAxisLeftMouseUpCallback(uniformLeftMouseUpCallback);
        scaleManipulators->InstallAxisMouseMoveCallback(uniformLeftMouseMoveCallback);
        scaleManipulators->InstallUniformLeftMouseDownCallback(uniformLeftMouseDownCallback);
        scaleManipulators->InstallUniformLeftMouseUpCallback(uniformLeftMouseUpCallback);
        scaleManipulators->InstallUniformMouseMoveCallback(uniformLeftMouseMoveCallback);

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(scaleManipulators);
    }

    void EditorTransformComponentSelection::InitializeManipulators(Manipulators& manipulators)
    {
        manipulators.Register(g_mainManipulatorManagerId);

        // camera editor component might have been set
        if (m_editorCameraComponentEntityId.IsValid())
        {
            for (AZ::EntityId entityId : m_selectedEntityIds)
            {
                // if so, ensure we do not register it with the manipulators
                if (entityId != m_editorCameraComponentEntityId)
                {
                    if (IsSelectableInViewport(entityId))
                    {
                        const AZ::ComponentId transformComponentId = GetTransformComponentId(entityId);
                        if (transformComponentId != AZ::InvalidComponentId)
                        {
                            manipulators.AddEntityComponentIdPair(AZ::EntityComponentIdPair(entityId, transformComponentId));
                            m_entityIdManipulators.m_lookups.insert_key(entityId);
                        }
                    }
                }
            }
        }
        else
        {
            // common case - editor camera component not set, ignore check
            for (AZ::EntityId entityId : m_selectedEntityIds)
            {
                if (IsSelectableInViewport(entityId))
                {
                    const AZ::ComponentId transformComponentId = GetTransformComponentId(entityId);
                    if (transformComponentId != AZ::InvalidComponentId)
                    {
                        manipulators.AddEntityComponentIdPair(AZ::EntityComponentIdPair(entityId, transformComponentId));
                        m_entityIdManipulators.m_lookups.insert_key(entityId);
                    }
                }
            }
        }
    }

    void EditorTransformComponentSelection::DeselectEntities()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!UndoRedoOperationInProgress())
        {
            ScopedUndoBatch undoBatch(EntitiesDeselectUndoRedoDesc);

            // restore manipulator overrides when undoing
            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // select must happen after to ensure in the undo/redo step the selection command
            // happens before the manipulator command
            auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), EntitiesDeselectUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();
        }

        m_selectedEntityIds.clear();
        SetSelectedEntities(EntityIdList());

        DestroyManipulators(m_entityIdManipulators);
        m_pivotOverrideFrame.Reset();
    }

    bool EditorTransformComponentSelection::SelectDeselect(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (entityId.IsValid())
        {
            if (IsEntitySelectedInternal(entityId, m_selectedEntityIds))
            {
                if (!UndoRedoOperationInProgress())
                {
                    RemoveEntityFromSelection(entityId);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(EntityDeselectUndoRedoDesc);

                    // store manipulator state when removing last entity from selection
                    if (m_entityIdManipulators.m_manipulators && nextEntityIds.empty())
                    {
                        CreateEntityManipulatorDeselectCommand(undoBatch);
                    }

                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, EntityDeselectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }
            else
            {
                if (!UndoRedoOperationInProgress())
                {
                    AddEntityToSelection(entityId);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(EntitySelectUndoRedoDesc);
                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, EntitySelectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }

            return true;
        }

        return false;
    }

    void EditorTransformComponentSelection::ChangeSelectedEntity(const AZ::EntityId entityId)
    {
        AZ_Assert(
            !UndoRedoOperationInProgress(),
            "ChangeSelectedEntity called from undo/redo operation - this is unexpected and not currently supported");

        // ensure deselect/select is tracked as an atomic undo/redo operation
        ScopedUndoBatch undoBatch(ChangeEntitySelectionUndoRedoDesc);

        DeselectEntities();
        SelectDeselect(entityId);
    }

    bool EditorTransformComponentSelection::HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        CheckDirtyEntityIds();

        const AzFramework::ViewportId viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
        const AzFramework::CameraState cameraState = GetCameraState(viewportId);

        const auto cursorEntityIdQuery = m_editorHelpers->FindEntityIdUnderCursor(cameraState, mouseInteraction);
        m_currentEntityIdUnderCursor = cursorEntityIdQuery.ContainerAncestorEntityId();

        const auto selectClickEvent = ClickDetectorEventFromViewportInteraction(mouseInteraction);
        m_cursorState.SetCurrentPosition(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
        const auto clickOutcome = m_clickDetector.DetectClick(selectClickEvent, m_cursorState.CursorDelta());

        // for entities selected with no bounds of their own (just TransformComponent)
        // check selection against the selection indicator aabb
        for (const AZ::EntityId& entityId : m_selectedEntityIds)
        {
            if (const auto entityIndex = SelectableInVisibleViewportCache(*m_entityDataCache, entityId); entityIndex.has_value())
            {
                const AZ::Transform& worldFromLocal = m_entityDataCache->GetVisibleEntityTransform(*entityIndex);
                const AZ::Vector3 boxPosition = worldFromLocal.TransformPoint(CalculateCenterOffset(entityId, m_pivotMode));
                const AZ::Vector3 scaledSize =
                    AZ::Vector3(PivotSize) * CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

                if (AabbIntersectMouseRay(
                        mouseInteraction.m_mouseInteraction,
                        AZ::Aabb::CreateFromMinMax(boxPosition - scaledSize, boxPosition + scaledSize)))
                {
                    m_currentEntityIdUnderCursor = entityId;
                }
            }
        }

        EditorContextMenuUpdate(m_contextMenu, mouseInteraction);

        if (m_boxSelect.HandleMouseInteraction(mouseInteraction))
        {
            return true;
        }

        if (Input::CycleManipulator(mouseInteraction))
        {
            const size_t scrollBound = 2;
            const auto nextMode =
                (static_cast<int>(m_mode) + scrollBound + (MouseWheelDelta(mouseInteraction) < 0.0f ? 1 : -1)) % scrollBound;

            SetTransformMode(static_cast<Mode>(nextMode));

            return true;
        }

        const AZ::EntityId entityIdUnderCursor = m_currentEntityIdUnderCursor;

        if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick &&
            mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
        {
            if (cursorEntityIdQuery.HasContainerAncestorEntityId())
            {
                if (auto prefabFocusInterface = AZ::Interface<Prefab::PrefabFocusInterface>::Get())
                {
                    prefabFocusInterface->FocusOnPrefabInstanceOwningEntityId(cursorEntityIdQuery.ContainerAncestorEntityId());
                    return false;
                }
            }
        }

        bool stickySelect = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            stickySelect, viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::StickySelectEnabled);

        if (stickySelect)
        {
            // double click to deselect all
            if (Input::DeselectAll(mouseInteraction))
            {
                // note: even if m_selectedEntityIds is technically empty, we
                // may still have an entity selected that was clicked in the
                // entity outliner - we still want to make sure the deselect all
                // action clears the selection
                DeselectEntities();
                return false;
            }
        }

        // select/deselect (add/remove) entities with ctrl held
        if (Input::AdditiveIndividualSelect(clickOutcome, mouseInteraction))
        {
            if (SelectDeselect(entityIdUnderCursor))
            {
                if (m_selectedEntityIds.empty())
                {
                    m_pivotOverrideFrame.Reset();
                }

                return false;
            }
        }

        if (!m_selectedEntityIds.empty())
        {
            // try snapping to a surface (mesh) if in Translation mode
            if (Input::SnapSurface(mouseInteraction))
            {
                PerformSnapToSurface(mouseInteraction);
                return false;
            }

            // group copying/alignment to specific entity - 'ditto' position/orientation for group
            if (Input::GroupDitto(mouseInteraction) && PerformGroupDitto(entityIdUnderCursor))
            {
                return false;
            }

            // individual copying/alignment to specific entity - 'ditto' position/orientation for individual
            if (Input::IndividualDitto(mouseInteraction) && PerformIndividualDitto(entityIdUnderCursor))
            {
                return false;
            }

            // set manipulator pivot override translation or orientation (update manipulators)
            const bool manipulatorDitto = Input::ManipulatorDitto(clickOutcome, mouseInteraction);
            const bool manipulatorDittoExact = Input::ManipulatorDittoExact(clickOutcome, mouseInteraction);
            if (manipulatorDitto || manipulatorDittoExact)
            {
                PerformManipulatorDitto(
                    entityIdUnderCursor,
                    [manipulatorDitto, manipulatorDittoExact, &mouseInteraction,
                     entityId = entityIdUnderCursor]() -> AZStd::optional<AZ::Vector3>
                    {
                        if (manipulatorDitto)
                        {
                            if (entityId.IsValid())
                            {
                                return GetWorldTranslation(entityId);
                            }

                            return AZStd::nullopt;
                        }

                        if (manipulatorDittoExact)
                        {
                            return EtcsPickEntity(entityId, mouseInteraction);
                        }

                        return AZStd::nullopt;
                    });

                return false;
            }

            if (stickySelect)
            {
                return false;
            }
        }

        // standard toggle selection
        if (Input::IndividualSelect(clickOutcome))
        {
            if (!stickySelect)
            {
                ChangeSelectedEntity(entityIdUnderCursor);
            }
            else
            {
                SelectDeselect(entityIdUnderCursor);
            }
        }

        return false;
    }

    bool EditorTransformComponentSelection::PerformGroupDitto(const AZ::EntityId entityId)
    {
        if (entityId.IsValid())
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            switch (m_mode)
            {
            case Mode::Rotation:
                CopyOrientationToSelectedEntitiesGroup(QuaternionFromTransformNoScaling(worldFromLocal));
                break;
            case Mode::Scale:
                CopyScaleToSelectedEntitiesIndividualWorld(worldFromLocal.GetUniformScale());
                break;
            case Mode::Translation:
                CopyTranslationToSelectedEntitiesGroup(worldFromLocal.GetTranslation());
                break;
            default:
                // do nothing
                break;
            }

            return true;
        }

        return false;
    }

    bool EditorTransformComponentSelection::PerformIndividualDitto(const AZ::EntityId entityId)
    {
        if (entityId.IsValid())
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            switch (m_mode)
            {
            case Mode::Rotation:
                CopyOrientationToSelectedEntitiesIndividual(QuaternionFromTransformNoScaling(worldFromLocal));
                break;
            case Mode::Scale:
                CopyScaleToSelectedEntitiesIndividualWorld(worldFromLocal.GetUniformScale());
                break;
            case Mode::Translation:
                CopyTranslationToSelectedEntitiesIndividual(worldFromLocal.GetTranslation());
                break;
            default:
                // do nothing
                break;
            }

            return true;
        }

        return false;
    }

    void EditorTransformComponentSelection::PerformSnapToSurface(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        for (const AZ::EntityId& entityId : m_selectedEntityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);
        }

        if (m_mode == Mode::Translation)
        {
            const AZ::Vector3 worldPosition = FindClosestPickIntersection(
                mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId,
                mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates, AzToolsFramework::EditorPickRayLength,
                GetDefaultEntityPlacementDistance());

            // handle modifier alternatives
            if (Input::IndividualDitto(mouseInteraction))
            {
                CopyTranslationToSelectedEntitiesIndividual(worldPosition);
            }
            else if (Input::GroupDitto(mouseInteraction))
            {
                CopyTranslationToSelectedEntitiesGroup(worldPosition);
            }
        }
        else if (m_mode == Mode::Rotation)
        {
            // handle modifier alternatives
            if (Input::IndividualDitto(mouseInteraction))
            {
                CopyOrientationToSelectedEntitiesIndividual(AZ::Quaternion::CreateIdentity());
            }
            else if (Input::GroupDitto(mouseInteraction))
            {
                CopyOrientationToSelectedEntitiesGroup(AZ::Quaternion::CreateIdentity());
            }
        }
    }

    template<typename ManipulatorTranslationFn>
    void EditorTransformComponentSelection::PerformManipulatorDitto(
        const AZ::EntityId entityId, ManipulatorTranslationFn&& manipulatorTranslationFn)
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(DittoManipulatorUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            auto overrideManipulatorTranslationFn = [this, &manipulatorTranslationFn]
            {
                if (const auto translation = manipulatorTranslationFn(); translation.has_value())
                {
                    OverrideManipulatorTranslation(translation.value());
                }
            };

            if (entityId.IsValid())
            {
                // set orientation/translation to match picked entity
                switch (m_mode)
                {
                case Mode::Rotation:
                    {
                        AZ::Quaternion worldOrientation = AZ::Quaternion::CreateIdentity();
                        AZ::TransformBus::EventResult(worldOrientation, entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
                        OverrideManipulatorOrientation(worldOrientation.GetNormalized());
                    }
                    break;
                case Mode::Translation:
                    overrideManipulatorTranslationFn();
                    break;
                case Mode::Scale:
                    // do nothing
                    break;
                default:
                    break;
                }
            }
            else
            {
                switch (m_mode)
                {
                case Mode::Translation:
                    overrideManipulatorTranslationFn();
                    break;
                case Mode::Rotation:
                case Mode::Scale:
                    // do nothing
                    break;
                default:
                    break;
                }
            }

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                m_entityIdManipulators.m_manipulators->GetLocalTransform()));

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    template<typename T>
    static void AddAction(
        AZStd::vector<AZStd::unique_ptr<QAction>>& actions,
        const QList<QKeySequence>& keySequences,
        AZ::Crc32 actionId,
        const QString& name,
        const QString& statusTip,
        const T& callback)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        actions.emplace_back(AZStd::make_unique<QAction>(nullptr));

        actions.back()->setShortcuts(keySequences);
        actions.back()->setText(name);
        actions.back()->setStatusTip(statusTip);

        QObject::connect(actions.back().get(), &QAction::triggered, actions.back().get(), callback);

        EditorActionRequestBus::Broadcast(&EditorActionRequests::AddActionViaBusCrc, actionId, actions.back().get());
    }

    // helper to enumerate all scene/level entities (will filter out system entities)
    template<typename Func>
    static void EnumerateEditorEntities(const Func& func)
    {
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::EnumerateEntities,
            [&func](const AZ::Entity* entity)
            {
                const AZ::EntityId entityId = entity->GetId();

                bool editorEntity = false;
                EditorEntityContextRequestBus::BroadcastResult(editorEntity, &EditorEntityContextRequests::IsEditorEntity, entityId);

                if (editorEntity)
                {
                    func(entityId);
                }
            });
    }

    void EditorTransformComponentSelection::DelegateClearManipulatorOverride()
    {
        switch (m_mode)
        {
        case Mode::Rotation:
            ClearManipulatorOrientationOverride();
            break;
        case Mode::Translation:
            ClearManipulatorTranslationOverride();
            break;
        case Mode::Scale:
            SetManipulatorViewBaseScale(DefaultManipulatorViewBaseScale);
            break;
        }
    }

    void EditorTransformComponentSelection::OnActionUpdaterRegistrationHook()
    {
        m_actionManagerInterface->RegisterActionUpdater(TransformModeChangedUpdaterIdentifier);
    }

    void EditorTransformComponentSelection::OnActionRegistrationHook()
    {
        RegisterActions();
    }
    
    void EditorTransformComponentSelection::RegisterActions()
    {
        using namespace EditorTransformComponentSelectionInternal;

        auto actionManager = AZ::Interface<ActionManagerInterface>::Get();
        auto hotkeyManager = AZ::Interface<HotKeyManagerInterface>::Get();

        auto IsInEditorPickMode = []() -> bool
        {
            if (auto viewportEditorModeTracker = AZ::Interface<AzToolsFramework::ViewportEditorModeTrackerInterface>::Get())
            {
                auto viewportEditorModes = viewportEditorModeTracker->GetViewportEditorModes({ AzToolsFramework::GetEntityContextId() });
                if (viewportEditorModes->IsModeActive(AzToolsFramework::ViewportEditorMode::Pick))
                {
                    return true;
                }
            }

            return false;
        };

        // Duplicate
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.duplicate";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = DuplicateTitle;
            actionProperties.m_description = DuplicateDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    auto readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
                    if (!readOnlyEntityPublicInterface)
                    {
                        return;
                    }

                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Don't allow duplication if any of the selected entities are direct descendants of a read-only entity
                    for (const auto& entityId : selectedEntities)
                    {
                        AZ::EntityId parentEntityId;
                        AZ::TransformBus::EventResult(parentEntityId, entityId, &AZ::TransformBus::Events::GetParentId);

                        if (parentEntityId.IsValid() && readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
                        {
                            return;
                        }
                    }

                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    ScopedUndoBatch undoBatch(DuplicateUndoRedoDesc);
                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), DuplicateUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    bool handled = false;
                    EditorRequestBus::Broadcast(&EditorRequests::CloneSelection, handled);

                    // selection update handled in AfterEntitySelectionChanged
                }
            );

            actionManager->InstallEnabledStateCallback(
                actionIdentifier,
                []() -> bool
                {
                    auto readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
                    if (!readOnlyEntityPublicInterface)
                    {
                        return true;
                    }

                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Don't allow duplication if any of the selected entities are direct descendants of a read-only entity
                    for (const auto& entityId : selectedEntities)
                    {
                        AZ::EntityId parentEntityId;
                        AZ::TransformBus::EventResult(parentEntityId, entityId, &AZ::TransformBus::Events::GetParentId);

                        if (parentEntityId.IsValid() && readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
                        {
                            return false;
                        }
                    }

                    return selectedEntities.size() > 0;
                }
            );

            // Trigger update whenever entity selection changes.
            actionManager->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+D");
        }

        // Delete
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.delete";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = DeleteTitle;
            actionProperties.m_description = DeleteDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [IsInEditorPickMode]()
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    // Don't allow users to delete entities while in Entity Picker mode.
                    if (IsInEditorPickMode())
                    {
                        return;
                    }

                    ScopedUndoBatch undoBatch(DeleteUndoRedoDesc);

                    instance->CreateEntityManipulatorDeselectCommand(undoBatch);

                    ToolsApplicationRequestBus::Broadcast(
                        &ToolsApplicationRequests::DeleteEntitiesAndAllDescendants, EntityIdVectorFromContainer(instance->m_selectedEntityIds));

                    instance->m_selectedEntityIds.clear();
                    instance->m_pivotOverrideFrame.Reset();
                }
            );

            actionManager->InstallEnabledStateCallback(
                actionIdentifier,
                [IsInEditorPickMode]() -> bool
                {
                    // Disable this action in Entity Picker mode.
                    if (IsInEditorPickMode())
                    {
                        return false;
                    }

                    auto readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
                    if (!readOnlyEntityPublicInterface)
                    {
                        return true;
                    }

                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Don't allow duplication if any of the selected entities are direct descendants of a read-only entity
                    for (const auto& entityId : selectedEntities)
                    {
                        AZ::EntityId parentEntityId;
                        AZ::TransformBus::EventResult(parentEntityId, entityId, &AZ::TransformBus::Events::GetParentId);

                        if (parentEntityId.IsValid() && readOnlyEntityPublicInterface->IsReadOnly(parentEntityId))
                        {
                            return false;
                        }
                    }

                    return selectedEntities.size() > 0;
                }
            );

            // Update this action's enabled state whenever entity selection changes, or entity pick mode is triggered.
            actionManager->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);
            actionManager->AddActionToUpdater(EditorIdentifiers::EntityPickingModeChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Delete");
        }

        // Select All
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.selectAll";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = SelectAllTitle;
            actionProperties.m_description = SelectAllDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    ScopedUndoBatch undoBatch(SelectAllEntitiesUndoRedoDesc);

                    if (instance->m_entityIdManipulators.m_manipulators)
                    {
                        auto manipulatorCommand =
                            AZStd::make_unique<EntityManipulatorCommand>(instance->CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

                        // note, nothing will change that the manipulatorCommand needs to keep track
                        // for after so no need to call SetManipulatorAfter

                        manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                        manipulatorCommand.release();
                    }

                    EnumerateEditorEntities(
                        [instance](AZ::EntityId entityId)
                        {
                            if (IsSelectableInViewport(entityId))
                            {
                                instance->AddEntityToSelection(entityId);
                            }
                        }
                    );

                    auto nextEntityIds = EntityIdVectorFromContainer(instance->m_selectedEntityIds);

                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, SelectAllEntitiesUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    instance->SetSelectedEntities(nextEntityIds);
                    instance->RegenerateManipulators();
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+A");
        }

        // Deselect All
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.deselectAll";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Deselect All";
            actionProperties.m_description = "Deselect All Entities";
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AzToolsFramework::EditorTransformComponentSelectionRequestBus::Event(
                        AzToolsFramework::GetEntityContextId(),
                        &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::DeselectEntities);
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Esc");
        }

        // Invert Selection
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.invertSelection";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = InvertSelectionTitle;
            actionProperties.m_description = InvertSelectionDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    ScopedUndoBatch undoBatch(InvertSelectionUndoRedoDesc);

                    if (instance->m_entityIdManipulators.m_manipulators)
                    {
                        auto manipulatorCommand =
                            AZStd::make_unique<EntityManipulatorCommand>(instance->CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

                        // note, nothing will change that the manipulatorCommand needs to keep track
                        // for after so no need to call SetManipulatorAfter

                        manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                        manipulatorCommand.release();
                    }

                    EntityIdSet entityIds;
                    EnumerateEditorEntities(
                        [instance, &entityIds](AZ::EntityId entityId)
                        {
                            const auto entityIdIt = AZStd::find(instance->m_selectedEntityIds.begin(), instance->m_selectedEntityIds.end(), entityId);
                            if (entityIdIt == instance->m_selectedEntityIds.end())
                            {
                                if (IsSelectableInViewport(entityId))
                                {
                                    entityIds.insert(entityId);
                                }
                            }
                        });

                    instance->m_selectedEntityIds = entityIds;

                    auto nextEntityIds = EntityIdVectorFromContainer(entityIds);

                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, InvertSelectionUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    instance->SetSelectedEntities(nextEntityIds);
                    instance->RegenerateManipulators();
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+Shift+I");
        }

        // Toggle Pivot Location
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.togglePivot";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = TogglePivotTitleEditMenu;
            actionProperties.m_description = TogglePivotDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    instance->ToggleCenterPivotSelection();
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "P");
        }

        // Reset Entity Transform
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.resetEntityTransform";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = ResetEntityTransformTitle;
            actionProperties.m_description = ResetEntityTransformDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }
                    switch (instance->m_mode)
                    {
                    case Mode::Rotation:
                        instance->ResetOrientationForSelectedEntitiesLocal();
                        break;
                    case Mode::Scale:
                        instance->CopyScaleToSelectedEntitiesIndividualLocal(1.0f);
                        break;
                    case Mode::Translation:
                        instance->ResetTranslationForSelectedEntitiesLocal();
                        break;
                    }
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "R");
        }

        // Reset Manipulator
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.resetManipulator";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = ResetManipulatorTitle;
            actionProperties.m_description = ResetManipulatorDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }
                    instance->DelegateClearManipulatorOverride();
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+R");
        }

        const auto showHide = [](const bool show)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            EditorTransformComponentSelection* instance = GetCurrentInstance();
            if (!instance)
            {
                return;
            }

            ScopedUndoBatch undoBatch(HideSelectionUndoRedoDesc);

            if (instance->m_entityIdManipulators.m_manipulators)
            {
                instance->CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(instance->m_selectedEntityIds);
            for (AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityVisibility(entityId, show);
            }

            instance->RegenerateManipulators();
        };

        // Show Selection
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.showSelection";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = ShowSelectionTitle;
            actionProperties.m_description = ShowSelectionDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [showHide]()
                {
                    showHide(true);
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
        }

        // Hide Selection
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.hideSelection";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = HideSelectionTitle;
            actionProperties.m_description = HideSelectionDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [showHide]()
                {
                    showHide(false);
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "H");
        }

        // Show All
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.showAll";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = ShowAllTitle;
            actionProperties.m_description = ShowAllDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    ScopedUndoBatch undoBatch(ShowAllEntitiesUndoRedoDesc);

                    EnumerateEditorEntities(
                        [](const AZ::EntityId entityId)
                        {
                            ScopedUndoBatch::MarkEntityDirty(entityId);
                            SetEntityVisibility(entityId, true);
                        }
                    );
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+Shift+H");
        }

        const auto lockUnlock = [](const bool lock)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            EditorTransformComponentSelection* instance = GetCurrentInstance();
            if (!instance)
            {
                return;
            }

            ScopedUndoBatch undoBatch(LockSelectionUndoRedoDesc);

            if (instance->m_entityIdManipulators.m_manipulators)
            {
                instance->CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(instance->m_selectedEntityIds);
            for (AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityLockState(entityId, lock);
            }

            instance->RegenerateManipulators();
        };

        // Lock Selection
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.lockSelection";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = LockSelectionTitle;
            actionProperties.m_description = LockSelectionDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [lockUnlock]()
                {
                    lockUnlock(true);
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "L");
        }

        // Unlock Selection
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.unlockSelection";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = UnlockSelectionTitle;
            actionProperties.m_description = UnlockSelectionDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [lockUnlock]()
                {
                    lockUnlock(false);
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
        }

        // Unlock All Entities
        {
            const AZStd::string_view actionIdentifier = "o3de.action.edit.unlockAllEntities";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = UnlockAllTitle;
            actionProperties.m_description = UnlockAllDesc;
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AZ_PROFILE_FUNCTION(AzToolsFramework);

                    ScopedUndoBatch undoBatch(UnlockAllUndoRedoDesc);

                    EnumerateEditorEntities(
                        [](AZ::EntityId entityId)
                        {
                            ScopedUndoBatch::MarkEntityDirty(entityId);
                            SetEntityLockState(entityId, false);
                        }
                    );
                }
            );

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "Ctrl+Shift+L");
        }

        // Transform Mode - Move
        {
            AZStd::string actionIdentifier = "o3de.action.edit.transform.move";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Move";
            actionProperties.m_description = "Select and move selected object(s)";
            actionProperties.m_category = "Edit";
            actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Move.svg";

            actionManager->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    instance->SetTransformMode(Mode::Translation);
                },
                []() -> bool
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return false;
                    }

                    return instance->GetTransformMode() == Mode::Translation;
                }
            );

            // Update when the transform mode changes.
            actionManager->AddActionToUpdater(TransformModeChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "1");
        }

        // Transform Mode - Rotate
        {
            AZStd::string actionIdentifier = "o3de.action.edit.transform.rotate";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Rotate";
            actionProperties.m_description = "Select and rotate selected object(s)";
            actionProperties.m_category = "Edit";
            actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Rotate.svg";

            actionManager->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    instance->SetTransformMode(Mode::Rotation);
                },
                []() -> bool
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return false;
                    }
                    return instance->GetTransformMode() == Mode::Rotation;
                }
            );

            // Update when the transform mode changes.
            actionManager->AddActionToUpdater(TransformModeChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "2");
        }

        // Transform Mode - Scale
        {
            AZStd::string actionIdentifier = "o3de.action.edit.transform.scale";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Scale";
            actionProperties.m_description = "Select and scale selected object(s)";
            actionProperties.m_category = "Edit";
            actionProperties.m_iconPath = ":/stylesheet/img/UI20/toolbar/Scale.svg";

            actionManager->RegisterCheckableAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return;
                    }

                    instance->SetTransformMode(Mode::Scale);
                },
                []() -> bool
                {
                    EditorTransformComponentSelection* instance = GetCurrentInstance();
                    if (!instance)
                    {
                        return false;
                    }
                    return instance->GetTransformMode() == Mode::Scale;
                }
            );

            // Update when the transform mode changes.
            actionManager->AddActionToUpdater(TransformModeChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);

            hotkeyManager->SetActionHotKey(actionIdentifier, "3");
        }

        // Move Up
        {
            const AZStd::string_view actionIdentifier = "o3de.action.entitySorting.moveUp";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Move Up";
            actionProperties.m_description = "Move the current selection up one row.";
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Can only move one entity at a time
                    if (selectedEntities.size() != 1)
                    {
                        return;
                    }

                    // Retrieve this entity's parent.
                    AZ::EntityId parentId;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                        parentId, selectedEntities.front(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

                    AzToolsFramework::ScopedUndoBatch undo("Move Entity Up");

                    EditorEntitySortRequestBus::Event(parentId, &EditorEntitySortRequestBus::Events::MoveChildEntityUp, selectedEntities.front());
                }
            );

            actionManager->InstallEnabledStateCallback(
                actionIdentifier,
                []() -> bool
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Can only move one entity at a time
                    if (selectedEntities.size() != 1)
                    {
                        return false;
                    }

                    // Retrieve this entity's parent.
                    AZ::EntityId parentId;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                        parentId, selectedEntities.front(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

                    bool result = false;
                    EditorEntitySortRequestBus::EventResult(
                        result, parentId, &EditorEntitySortRequestBus::Events::CanMoveChildEntityUp, selectedEntities.front());

                    return result;
                }
            );

            // Trigger update whenever entity selection changes.
            actionManager->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
        }

        // Move Down
        {
            const AZStd::string_view actionIdentifier = "o3de.action.entitySorting.moveDown";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Move Down";
            actionProperties.m_description = "Move the current selection down one row.";
            actionProperties.m_category = "Edit";

            actionManager->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []()
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Can only move one entity at a time
                    if (selectedEntities.size() != 1)
                    {
                        return;
                    }

                    // Retrieve this entity's parent.
                    AZ::EntityId parentId;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                        parentId, selectedEntities.front(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

                    AzToolsFramework::ScopedUndoBatch undo("Move Entity Down");

                    EditorEntitySortRequestBus::Event(
                        parentId, &EditorEntitySortRequestBus::Events::MoveChildEntityDown, selectedEntities.front());
                }
            );

            actionManager->InstallEnabledStateCallback(
                actionIdentifier,
                []() -> bool
                {
                    AzToolsFramework::EntityIdList selectedEntities;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                    // Can only move one entity at a time
                    if (selectedEntities.size() != 1)
                    {
                        return false;
                    }

                    // Retrieve this entity's parent.
                    AZ::EntityId parentId;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                        parentId, selectedEntities.front(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetParent);

                    bool result = false;
                    EditorEntitySortRequestBus::EventResult(
                        result, parentId, &EditorEntitySortRequestBus::Events::CanMoveChildEntityDown, selectedEntities.front());

                    return result;
                }
            );

            // Trigger update whenever entity selection changes.
            actionManager->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManager->AssignModeToAction(DefaultActionContextModeIdentifier, actionIdentifier);
        }
    }

    void EditorTransformComponentSelection::OnMenuBindingHook()
    {
        // Edit Menu
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EditMenuIdentifier, 300);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.duplicate", 400);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.delete", 500);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EditMenuIdentifier, 600);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.selectAll", 700);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.deselectAll", 750);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.invertSelection", 800);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EditMenuIdentifier, 900);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.togglePivot", 1000);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.resetEntityTransform", 1100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.resetManipulator", 1200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.hideSelection", 1300);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.showAll", 1400);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.lockSelection", 1500);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.edit.unlockAllEntities", 1600);
        m_menuManagerInterface->AddSeparatorToMenu(EditorIdentifiers::EditMenuIdentifier, 1700);

        // Transform Modes
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditModifyModesMenuIdentifier, "o3de.action.edit.transform.move", 100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditModifyModesMenuIdentifier, "o3de.action.edit.transform.rotate", 200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditModifyModesMenuIdentifier, "o3de.action.edit.transform.scale", 300);

        // Entity Outliner Context Menu
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.edit.duplicate", 40100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.edit.delete", 40200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.edit.togglePivot", 60200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.entitySorting.moveUp", 70200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.entitySorting.moveDown", 70300);

        // Viewport Context Menu
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.edit.duplicate", 40100);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.edit.delete", 40200);
        m_menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.edit.togglePivot", 60200);
    }

    void EditorTransformComponentSelection::UnregisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Unregister();
        }
    }

    void EditorTransformComponentSelection::RegisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && !m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Register(g_mainManipulatorManagerId);
        }
    }

    void EditorTransformComponentSelection::CreateEntityIdManipulators()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_selectedEntityIds.empty())
        {
            return;
        }

        switch (m_mode)
        {
        case Mode::Translation:
            CreateTranslationManipulators();
            break;
        case Mode::Rotation:
            CreateRotationManipulators();
            break;
        case Mode::Scale:
            CreateScaleManipulators();
            break;
        }

        // workaround to ensure we never show a manipulator that is bound to no
        // entities (may happen if entities are locked/hidden but currently selected)
        if (m_entityIdManipulators.m_lookups.empty())
        {
            DestroyManipulators(m_entityIdManipulators);
        }
    }

    void EditorTransformComponentSelection::RegenerateManipulators()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // do not create manipulators for the container entity of the focused prefab.
        if (auto prefabFocusPublicInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusPublicInterface>::Get())
        {
            AzFramework::EntityContextId editorEntityContextId = GetEntityContextId();
            if (AZ::EntityId focusRoot = prefabFocusPublicInterface->GetFocusedPrefabContainerEntityId(editorEntityContextId);
                focusRoot.IsValid())
            {
                m_selectedEntityIds.erase(focusRoot);
            }
        }

        // do not create manipulators for any entities marked as read only
        if (auto readOnlyEntityPublicInterface = AZ::Interface<ReadOnlyEntityPublicInterface>::Get())
        {
            AZStd::erase_if(
                m_selectedEntityIds,
                [readOnlyEntityPublicInterface](auto entityId)
                {
                    return readOnlyEntityPublicInterface->IsReadOnly(entityId);
                });
        }

        // note: create/destroy pattern to be addressed
        DestroyManipulators(m_entityIdManipulators);
        CreateEntityIdManipulators();
    }

    void EditorTransformComponentSelection::CreateTransformModeSelectionCluster()
    {
        // create the cluster for changing transform mode
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_transformModeClusterId, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopLeft);

        // create and register the buttons (strings correspond to icons even if the values appear different)
        m_translateButtonId = RegisterClusterButton(m_transformModeClusterId, "Move");
        m_rotateButtonId = RegisterClusterButton(m_transformModeClusterId, "Rotate");
        m_scaleButtonId = RegisterClusterButton(m_transformModeClusterId, "Scale");

        // set button tooltips
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_transformModeClusterId,
            m_translateButtonId, TransformModeClusterTranslateTooltip);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_transformModeClusterId,
            m_rotateButtonId, TransformModeClusterRotateTooltip);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_transformModeClusterId,
            m_scaleButtonId, TransformModeClusterScaleTooltip);

        auto onButtonClicked = [this](ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_translateButtonId)
            {
                SetTransformMode(Mode::Translation);
            }
            else if (buttonId == m_rotateButtonId)
            {
                SetTransformMode(Mode::Rotation);
            }
            else if (buttonId == m_scaleButtonId)
            {
                SetTransformMode(Mode::Scale);
            }
        };

        m_transformModeSelectionHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onButtonClicked);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_transformModeClusterId,
            m_translateButtonId);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_transformModeClusterId,
            m_transformModeSelectionHandler);
    }

    void EditorTransformComponentSelection::CreateSnappingCluster()
    {
        // create the cluster for switching spaces/reference frames
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_snappingCluster.m_clusterId, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopRight);

        m_snappingCluster.m_snapToWorldButtonId = RegisterClusterButton(m_snappingCluster.m_clusterId, "Grid");

        // set button tooltips
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip,
            m_snappingCluster.m_clusterId, m_snappingCluster.m_snapToWorldButtonId, SnappingClusterSnapToWorldTooltip);

        const auto onButtonClicked = [this](const ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_snappingCluster.m_snapToWorldButtonId)
            {
                float gridSize = 1.0f;
                ViewportInteraction::ViewportSettingsRequestBus::EventResult(
                    gridSize, ViewportUi::DefaultViewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::GridSize);

                SnapSelectedEntitiesToWorldGrid(gridSize);
            }
        };

        m_snappingCluster.m_snappingHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onButtonClicked);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_snappingCluster.m_clusterId, m_snappingCluster.m_snappingHandler);

        // hide initially
        SetViewportUiClusterVisible(m_snappingCluster.m_clusterId, false);
    }

    void EditorTransformComponentSelection::CreateSpaceSelectionCluster()
    {
        // create the cluster for switching spaces/reference frames
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_spaceCluster.m_clusterId, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopRight);

        // create and register the buttons (strings correspond to icons even if the values appear different)
        m_spaceCluster.m_worldButtonId = RegisterClusterButton(m_spaceCluster.m_clusterId, "World");
        m_spaceCluster.m_parentButtonId = RegisterClusterButton(m_spaceCluster.m_clusterId, "Parent");
        m_spaceCluster.m_localButtonId = RegisterClusterButton(m_spaceCluster.m_clusterId, "Local");

        // set button tooltips
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_spaceCluster.m_clusterId,
            m_spaceCluster.m_worldButtonId, SpaceClusterWorldTooltip);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_spaceCluster.m_clusterId,
            m_spaceCluster.m_parentButtonId, SpaceClusterParentTooltip);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, m_spaceCluster.m_clusterId,
            m_spaceCluster.m_localButtonId, SpaceClusterLocalTooltip);

        auto onButtonClicked = [this](ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_spaceCluster.m_localButtonId)
            {
                // Unlock
                if (m_spaceCluster.m_spaceLock.has_value() && m_spaceCluster.m_spaceLock.value() == ReferenceFrame::Local)
                {
                    m_spaceCluster.m_spaceLock = AZStd::nullopt;
                }
                else
                {
                    m_spaceCluster.m_spaceLock = ReferenceFrame::Local;
                }
            }
            else if (buttonId == m_spaceCluster.m_parentButtonId)
            {
                // Unlock
                if (m_spaceCluster.m_spaceLock.has_value() && m_spaceCluster.m_spaceLock.value() == ReferenceFrame::Parent)
                {
                    m_spaceCluster.m_spaceLock = AZStd::nullopt;
                }
                else
                {
                    m_spaceCluster.m_spaceLock = ReferenceFrame::Parent;
                }
            }
            else if (buttonId == m_spaceCluster.m_worldButtonId)
            {
                // Unlock
                if (m_spaceCluster.m_spaceLock.has_value() && m_spaceCluster.m_spaceLock.value() == ReferenceFrame::World)
                {
                    m_spaceCluster.m_spaceLock = AZStd::nullopt;
                }
                else
                {
                    m_spaceCluster.m_spaceLock = ReferenceFrame::World;
                }
            }
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonLocked,
                m_spaceCluster.m_clusterId, buttonId, m_spaceCluster.m_spaceLock.has_value());
        };

        m_spaceCluster.m_spaceHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onButtonClicked);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_spaceCluster.m_clusterId, m_spaceCluster.m_spaceHandler);
    }

    void EditorTransformComponentSelection::CreateComponentModeSwitcher()
    {
        if (ComponentSwitcherEnabled())
        {
            m_componentModeSwitcher = AZStd::make_shared<ComponentModeFramework::ComponentModeSwitcher>();
        }
    }

    void EditorTransformComponentSelection::OverrideComponentModeSwitcher(
        AZStd::shared_ptr<ComponentModeFramework::ComponentModeSwitcher> componentModeSwitcher)
    {
        m_componentModeSwitcher = componentModeSwitcher;
    }

    void EditorTransformComponentSelection::SnapSelectedEntitiesToWorldGrid(const float gridSize)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const AZStd::array snapAxes = { AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ() };

        ScopedUndoBatch undoBatch(SnapToWorldGridUndoRedoDesc);
        for (const AZ::EntityId& entityId : m_selectedEntityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);
            SetEntityWorldTranslation(
                entityId, CalculateSnappedPosition(GetWorldTranslation(entityId), snapAxes.data(), snapAxes.size(), gridSize));
        }

        RefreshManipulators(RefreshType::Translation);
    }

    EditorTransformComponentSelectionRequests::Mode EditorTransformComponentSelection::GetTransformMode()
    {
        return m_mode;
    }

    void EditorTransformComponentSelection::SetTransformMode(const Mode mode)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (mode == m_mode)
        {
            return;
        }

        if (m_pivotOverrideFrame.m_orientationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_orientationOverride = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetRotation();
        }

        if (m_pivotOverrideFrame.m_translationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_translationOverride = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
        }

        m_mode = mode;

        // Update Transform Mode Actions.
        if (m_actionManagerInterface)
        {
            m_actionManagerInterface->TriggerActionUpdater(TransformModeChangedUpdaterIdentifier);
        }

        // Set the corresponding Viewport UI button to active.
        switch (mode)
        {
        case Mode::Translation:
            SetViewportUiClusterActiveButton(m_transformModeClusterId, m_translateButtonId);
            break;
        case Mode::Rotation:
            SetViewportUiClusterActiveButton(m_transformModeClusterId, m_rotateButtonId);
            break;
        case Mode::Scale:
            SetViewportUiClusterActiveButton(m_transformModeClusterId, m_scaleButtonId);
            break;
        }

        RegenerateManipulators();
    }

    void EditorTransformComponentSelection::UndoRedoEntityManipulatorCommand(const AZ::u8 pivotOverride, const AZ::Transform& transform)
    {
        m_pivotOverrideFrame.Reset();

        RefreshSelectedEntityIdsAndRegenerateManipulators();

        if (m_entityIdManipulators.m_manipulators && PivotHasTransformOverride(pivotOverride))
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(transform);

            if (PivotHasOrientationOverride(pivotOverride))
            {
                m_pivotOverrideFrame.m_orientationOverride = QuaternionFromTransformNoScaling(transform);
            }

            if (PivotHasTranslationOverride(pivotOverride))
            {
                m_pivotOverrideFrame.m_translationOverride = transform.GetTranslation();
            }
        }
    }

    void EditorTransformComponentSelection::AddEntityToSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_selectedEntityIds.insert(entityId);

        AZ::TransformNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EditorTransformComponentSelection::RemoveEntityFromSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_selectedEntityIds.erase(entityId);

        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(entityId);
    }

    bool EditorTransformComponentSelection::IsEntitySelected(const AZ::EntityId entityId) const
    {
        return IsEntitySelectedInternal(entityId, m_selectedEntityIds);
    }

    void EditorTransformComponentSelection::SetSelectedEntities(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // we are responsible for updating the current selection
        m_didSetSelectedEntities = true;
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
    }

    void EditorTransformComponentSelection::RefreshManipulators(const RefreshType refreshType)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            if (!m_entityIdManipulators.m_manipulators->PerformingAction())
            {
                AZ::Transform transform;
                switch (refreshType)
                {
                case RefreshType::All:
                    transform = RecalculateAverageManipulatorTransform(
                        m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame);
                    break;
                case RefreshType::Orientation:
                    transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        RecalculateAverageManipulatorOrientation(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_referenceFrame),
                        m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());
                    break;
                case RefreshType::Translation:
                    transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        m_entityIdManipulators.m_manipulators->GetLocalTransform().GetRotation(),
                        RecalculateAverageManipulatorTranslation(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode));
                    break;
                }

                m_entityIdManipulators.m_manipulators->SetLocalTransform(transform);

                m_triedToRefresh = false;
            }
            else
            {
                m_triedToRefresh = true;
            }
        }
    }

    void EditorTransformComponentSelection::OverrideManipulatorOrientation(const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_pivotOverrideFrame.m_orientationOverride = orientation;

        if (m_entityIdManipulators.m_manipulators)
        {
            m_entityIdManipulators.m_manipulators->SetLocalTransform(AZ::Transform::CreateFromQuaternionAndTranslation(
                orientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();
        }
    }

    void EditorTransformComponentSelection::OverrideManipulatorTranslation(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_pivotOverrideFrame.m_translationOverride = translation;

        if (m_entityIdManipulators.m_manipulators)
        {
            m_entityIdManipulators.m_manipulators->SetLocalPosition(translation);
            m_entityIdManipulators.m_manipulators->SetBoundsDirty();
        }
    }

    void EditorTransformComponentSelection::ClearManipulatorTranslationOverride()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(ResetManipulatorTranslationUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedTranslation();

            m_entityIdManipulators.m_manipulators->SetLocalTransform(RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    void EditorTransformComponentSelection::ClearManipulatorOrientationOverride()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch{ ResetManipulatorOrientationUndoRedoDesc };

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedOrientation();

            // parent reference frame is the default (when no modifiers are held)
            m_entityIdManipulators.m_manipulators->SetLocalTransform(AZ::Transform::CreateFromQuaternionAndTranslation(
                Etcs::CalculatePivotOrientationForEntityIds(m_entityIdManipulators.m_lookups, ReferenceFrame::Parent).m_worldOrientation,
                m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    void EditorTransformComponentSelection::ToggleCenterPivotSelection()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        m_pivotMode = TogglePivotMode(m_pivotMode);
        RefreshManipulators(RefreshType::Translation);
    }

    template<typename EntityIdMap>
    static bool ShouldUpdateEntityTransform(const AZ::EntityId entityId, const EntityIdMap& entityIdMap)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);
        while (parentId.IsValid())
        {
            const auto parentIt = entityIdMap.find(parentId);
            if (parentIt != entityIdMap.end())
            {
                return false;
            }

            const AZ::EntityId currentParentId = parentId;
            parentId.SetInvalid();
            AZ::TransformBus::EventResult(parentId, currentParentId, &AZ::TransformBus::Events::GetParentId);
        }

        return true;
    }

    void EditorTransformComponentSelection::CopyTranslationToSelectedEntitiesGroup(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(DittoTranslationGroupUndoRedoDesc);

            // store previous translation manipulator position
            const AZ::Vector3 previousPivotTranslation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation)));

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                // do not update position of child if parent/ancestor is in selection
                if (!ShouldUpdateEntityTransform(entityId, m_entityIdManipulators.m_lookups))
                {
                    continue;
                }

                const AZ::Vector3 entityTranslation = GetWorldTranslation(entityId);

                SetEntityWorldTranslation(entityId, translation + (entityTranslation - previousPivotTranslation));
            }

            RefreshManipulators(RefreshType::Translation);

            RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
        }
    }

    void EditorTransformComponentSelection::CopyTranslationToSelectedEntitiesIndividual(const AZ::Vector3& translation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(DittoTranslationIndividualUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation)));

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                SetEntityWorldTranslation(entityId, translation);
            }

            RefreshManipulators(RefreshType::Translation);

            RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
        }
    }

    void EditorTransformComponentSelection::CopyScaleToSelectedEntitiesIndividualWorld(float scale)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        ScopedUndoBatch undoBatch(DittoScaleIndividualWorldUndoRedoDesc);

        ManipulatorEntityIds manipulatorEntityIds;
        BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

        // save initial transforms - this is necessary in cases where entities exist
        // in a hierarchy. We want to make sure a parent transform does not affect
        // a child transform (entities seeming to get the same transform applied twice)
        const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

        // update scale relative to initial
        const AZ::Transform scaleTransform = AZ::Transform::CreateUniformScale(scale);
        for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);

            const auto transformIt = transformsBefore.find(entityId);
            if (transformIt != transformsBefore.end())
            {
                AZ::Transform transformBefore = transformIt->second;
                transformBefore.ExtractUniformScale();
                AZ::Transform newWorldFromLocal = transformBefore * scaleTransform;

                SetEntityWorldTransform(entityId, newWorldFromLocal);
            }
        }

        RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
    }

    void EditorTransformComponentSelection::CopyScaleToSelectedEntitiesIndividualLocal(float scale)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        ScopedUndoBatch undoBatch(DittoScaleIndividualLocalUndoRedoDesc);

        ManipulatorEntityIds manipulatorEntityIds;
        BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

        // update scale relative to initial
        for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
        {
            ScopedUndoBatch::MarkEntityDirty(entityId);
            SetEntityLocalScale(entityId, scale);
        }

        RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
    }

    void EditorTransformComponentSelection::CopyOrientationToSelectedEntitiesIndividual(const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch{ DittoEntityOrientationIndividualUndoRedoDesc };

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            // save initial transforms
            const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

            // update orientations relative to initial
            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                if (const auto transformIt = transformsBefore.find(entityId); transformIt != transformsBefore.end())
                {
                    ScopedUndoBatch::MarkEntityDirty(entityId);
                    SetEntityLocalRotation(entityId, orientation);
                }
            }

            OverrideManipulatorOrientation(orientation);

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
        }
    }

    void EditorTransformComponentSelection::CopyOrientationToSelectedEntitiesGroup(const AZ::Quaternion& orientation)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(DittoEntityOrientationGroupUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            // save initial transforms
            const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

            const AZ::Transform currentTransform = m_entityIdManipulators.m_manipulators->GetLocalTransform();
            const AZ::Transform nextTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                orientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());

            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                if (!ShouldUpdateEntityTransform(entityId, m_entityIdManipulators.m_lookups))
                {
                    continue;
                }

                const auto transformIt = transformsBefore.find(entityId);
                if (transformIt != transformsBefore.end())
                {
                    const AZ::Transform transformInPivotSpace = currentTransform.GetInverse() * transformIt->second;

                    SetEntityWorldTransform(entityId, nextTransform * transformInPivotSpace);
                }
            }

            OverrideManipulatorOrientation(orientation);

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();

            RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);
        }
    }

    void EditorTransformComponentSelection::ResetOrientationForSelectedEntitiesLocal()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        ScopedUndoBatch undoBatch(ResetOrientationToParentUndoRedoDesc);
        for (const auto& entityIdLookup : m_entityIdManipulators.m_lookups)
        {
            ScopedUndoBatch::MarkEntityDirty(entityIdLookup.first);
            SetEntityLocalRotation(entityIdLookup.first, AZ::Vector3::CreateZero());
        }

        ManipulatorEntityIds manipulatorEntityIds;
        BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

        RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);

        ClearManipulatorOrientationOverride();
    }

    void EditorTransformComponentSelection::ResetTranslationForSelectedEntitiesLocal()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(ResetTranslationToParentUndoRedoDesc);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                AZ::EntityId parentId;
                AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                if (parentId.IsValid())
                {
                    ScopedUndoBatch::MarkEntityDirty(entityId);
                    SetEntityLocalTranslation(entityId, AZ::Vector3::CreateZero());
                }
            }

            RefreshUiAfterChange(manipulatorEntityIds.m_entityIds);

            ClearManipulatorTranslationOverride();
        }
    }

    void EditorTransformComponentSelection::BeforeEntitySelectionChanged()
    {
        // EditorTransformComponentSelection was not responsible for the change in selection
        if (!m_didSetSelectedEntities)
        {
            if (!UndoRedoOperationInProgress())
            {
                // we know we will be tracking an undo step when selection changes so ensure
                // we also record the state of the manipulator before and after the change
                BeginRecordManipulatorCommand();

                // ensure we reset/refresh any pivot overrides when a selection
                // change originated outside EditorTransformComponentSelection
                m_pivotOverrideFrame.Reset();
            }
        }
    }

    void EditorTransformComponentSelection::AfterEntitySelectionChanged(
        [[maybe_unused]] const EntityIdList& newlySelectedEntities, [[maybe_unused]] const EntityIdList& newlyDeselectedEntities)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // EditorTransformComponentSelection was not responsible for the change in selection
        if (!m_didSetSelectedEntities)
        {
            if (!UndoRedoOperationInProgress())
            {
                EndRecordManipulatorCommand();
            }

            RefreshSelectedEntityIds();
        }
        else
        {
            // clear if we instigated the selection change after selection changes
            m_didSetSelectedEntities = false;
        }

        m_snappingCluster.TrySetVisible(m_viewportUiVisible && !m_selectedEntityIds.empty());

        RegenerateManipulators();
    }

    static void DrawPreviewAxis(
        AzFramework::DebugDisplayRequests& display,
        const AZ::Transform& transform,
        const float axisLength,
        const AzFramework::CameraState& cameraState)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const int prevState = display.GetState();

        display.DepthWriteOff();
        display.DepthTestOff();
        display.CullOff();

        display.SetLineWidth(4.0f);

        const auto axisFlip = [&transform, &cameraState](const AZ::Vector3& axis)
        {
            return ShouldFlipCameraAxis(
                       AZ::Transform::CreateIdentity(), transform.GetTranslation(), TransformDirectionNoScaling(transform, axis),
                       cameraState)
                ? -1.0f
                : 1.0f;
        };

        display.SetColor(FadedXAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafe() * axisLength * axisFlip(AZ::Vector3::CreateAxisX()));
        display.SetColor(FadedYAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafe() * axisLength * axisFlip(AZ::Vector3::CreateAxisY()));
        display.SetColor(FadedZAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisZ().GetNormalizedSafe() * axisLength * axisFlip(AZ::Vector3::CreateAxisZ()));

        display.DepthWriteOn();
        display.DepthTestOn();
        display.CullOn();
        display.SetLineWidth(1.0f);

        display.SetState(prevState);
    }

    static void DrawManipulatorGrid(
        AzFramework::DebugDisplayRequests& debugDisplay, const EntityIdManipulators& entityIdManipulators, const float gridSize)
    {
        const AZ::Matrix3x3 orientation = AZ::Matrix3x3::CreateFromTransform(entityIdManipulators.m_manipulators->GetLocalTransform());

        const AZ::Vector3 translation = entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();

        DrawSnappingGrid(debugDisplay, AZ::Transform::CreateFromMatrix3x3AndTranslation(orientation, translation), gridSize);
    }

    void EditorTransformComponentSelection::DisplayViewportSelection(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        CheckDirtyEntityIds();

        const auto keyboardModifiers = AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();

        m_cursorState.Update();

        bool stickySelect = false;
        ViewportInteraction::ViewportSettingsRequestBus::EventResult(
            stickySelect, viewportInfo.m_viewportId, &ViewportInteraction::ViewportSettingsRequestBus::Events::StickySelectEnabled);

        HandleAccentsContext handleAccentsContext;
        handleAccentsContext.m_ctrlHeld = keyboardModifiers.Ctrl();
        handleAccentsContext.m_hasSelectedEntities = !m_selectedEntityIds.empty();
        handleAccentsContext.m_usingBoxSelect = m_boxSelect.Active();
        handleAccentsContext.m_usingStickySelect = stickySelect;

        HandleAccents(
            m_currentEntityIdUnderCursor, m_hoveredEntityId, handleAccentsContext,
            ViewportInteraction::BuildMouseButtons(QGuiApplication::mouseButtons()),
            [](const AZ::EntityId entityId, bool highlighted)
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, entityId, highlighted);
            });

        const ReferenceFrame referenceFrame = m_spaceCluster.m_spaceLock.value_or(ReferenceFrameFromModifiers(keyboardModifiers));

        UpdateSpaceCluster(referenceFrame);

        bool refresh = false;
        if (referenceFrame != m_referenceFrame)
        {
            m_referenceFrame = referenceFrame;
            refresh = true;
        }

        refresh = refresh ||
            (m_triedToRefresh && m_entityIdManipulators.m_manipulators && !m_entityIdManipulators.m_manipulators->PerformingAction());

        // we've moved from parent to world space, parent to local space or vice versa by holding or
        // releasing shift and/or alt - make sure we update the manipulator orientation appropriately
        if (refresh)
        {
            RefreshManipulators(RefreshType::Orientation);
        }

        const AzFramework::CameraState cameraState = GetCameraState(viewportInfo.m_viewportId);

        const auto entityFilter = [this](AZ::EntityId entityId)
        {
            const bool entityHasManipulator = m_entityIdManipulators.m_lookups.find(entityId) != m_entityIdManipulators.m_lookups.end();

            return !entityHasManipulator;
        };

        m_editorHelpers->DisplayHelpers(viewportInfo, cameraState, debugDisplay, entityFilter);

        if (!m_selectedEntityIds.empty())
        {
            // check what pivot orientation we are in (based on if a modifier is
            // held to move us from parent to world space or parent to local space)
            // or if we set a pivot override
            const auto pivotResult =
                Etcs::CalculateSelectionPivotOrientation(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_referenceFrame);

            // if the reference frame was parent space and the selection does have a
            // valid parent, draw a preview axis at its position/orientation
            if (pivotResult.m_parentId.IsValid())
            {
                if (auto parentEntityIndex = m_entityDataCache->GetVisibleEntityIndexFromId(pivotResult.m_parentId))
                {
                    const AZ::Transform& worldFromLocal = m_entityDataCache->GetVisibleEntityTransform(*parentEntityIndex);

                    const float adjustedLineLength = CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

                    DrawPreviewAxis(debugDisplay, worldFromLocal, adjustedLineLength, cameraState);
                }
            }

            debugDisplay.DepthTestOff();
            debugDisplay.DepthWriteOff();

            for (AZ::EntityId entityId : m_selectedEntityIds)
            {
                if (auto entityIndex = m_entityDataCache->GetVisibleEntityIndexFromId(entityId))
                {
                    const AZ::Transform& worldFromLocal = m_entityDataCache->GetVisibleEntityTransform(*entityIndex);
                    const bool hidden = !m_entityDataCache->IsVisibleEntityVisible(*entityIndex);
                    const bool locked = m_entityDataCache->IsVisibleEntityLocked(*entityIndex);

                    const AZ::Vector3 boxPosition = worldFromLocal.TransformPoint(CalculateCenterOffset(entityId, m_pivotMode));

                    const AZ::Vector3 scaledSize =
                        AZ::Vector3(PivotSize) * CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

                    const AZ::Color hiddenNormal[] = { AzFramework::ViewportColors::SelectedColor,
                                                       AzFramework::ViewportColors::HiddenColor };
                    AZ::Color boxColor = hiddenNormal[hidden];
                    const AZ::Color lockedOther[] = { boxColor, AzFramework::ViewportColors::LockColor };
                    boxColor = lockedOther[locked];

                    debugDisplay.SetColor(boxColor);

                    // draw the pivot (origin) of the box
                    debugDisplay.DrawWireBox(boxPosition - scaledSize, boxPosition + scaledSize);
                }
            }

            debugDisplay.DepthWriteOn();
            debugDisplay.DepthTestOn();

            if (ShowingGrid(viewportInfo.m_viewportId) && m_mode == Mode::Translation && !ComponentModeFramework::InComponentMode())
            {
                const GridSnapParameters gridSnapParams = GridSnapSettings(viewportInfo.m_viewportId);
                if (gridSnapParams.m_gridSnap && m_entityIdManipulators.m_manipulators)
                {
                    DrawManipulatorGrid(debugDisplay, m_entityIdManipulators, gridSnapParams.m_gridSize);
                }
            }
        }

        // draw a preview axis of where the manipulator was when the action started
        // only do this while the manipulator is being interacted with
        if (m_entityIdManipulators.m_manipulators)
        {
            // if we have an active manipulator, ensure we refresh the view while drawing
            // in case it needs to be recalculated based on the current view position
            m_entityIdManipulators.m_manipulators->RefreshView(cameraState.m_position);
            // do any additional drawing for this aggregate manipulator/view pairing
            m_entityIdManipulators.m_manipulators->DisplayFeedback(debugDisplay, cameraState);

            if (m_entityIdManipulators.m_manipulators->PerformingAction())
            {
                const float adjustedLineLength = 2.0f * CalculateScreenToWorldMultiplier(m_axisPreview.m_translation, cameraState);

                DrawPreviewAxis(
                    debugDisplay,
                    AZ::Transform::CreateFromQuaternionAndTranslation(m_axisPreview.m_orientation, m_axisPreview.m_translation),
                    adjustedLineLength, cameraState);
            }
        }

        m_boxSelect.DisplayScene(viewportInfo, debugDisplay);
    }

    static void DrawAxisGizmo(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // get the editor cameras current orientation
        const int viewportId = viewportInfo.m_viewportId;
        const AzFramework::CameraState editorCameraState = GetCameraState(viewportId);
        const AZ::Matrix3x3& editorCameraOrientation = AZ::Matrix3x3::CreateFromMatrix3x4(AzFramework::CameraTransform(editorCameraState));

        // create a gizmo camera transform about the origin matching the orientation of the editor camera
        // (10 units back in the y axis to produce an orbit effect)
        const AZ::Transform gizmoCameraOffset = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(10.0f));
        const AZ::Transform gizmoCameraTransform = AZ::Transform::CreateFromMatrix3x3(editorCameraOrientation) * gizmoCameraOffset;
        const AzFramework::CameraState gizmoCameraState =
            AzFramework::CreateDefaultCamera(gizmoCameraTransform, editorCameraState.m_viewportSize);

        // cache the gizmo camera view and projection for world-to-screen calculations
        const auto cameraView = AzFramework::CameraView(gizmoCameraState);
        const auto cameraProjection = AzFramework::CameraProjection(gizmoCameraState);

        // screen space offset to move the 2d gizmo around
        const AZ::Vector2 screenOffset = AZ::Vector2(ed_viewportGizmoAxisScreenPosition) - AZ::Vector2(0.5f, 0.5f);

        // map from a position in world space (relative to the the gizmo camera near the origin) to a position in
        // screen space
        const auto calculateGizmoAxis = [&cameraView, &cameraProjection, &screenOffset](const AZ::Vector3& axis)
        {
            auto result = AZ::Vector2(AzFramework::WorldToScreenNdc(axis, cameraView, cameraProjection));
            result.SetY(1.0f - result.GetY());
            return result + screenOffset;
        };

        // get all important axis positions in screen space
        const float lineLength = ed_viewportGizmoAxisLineLength;
        const auto gizmoStart = calculateGizmoAxis(AZ::Vector3::CreateZero());
        const auto gizmoEndAxisX = calculateGizmoAxis(-AZ::Vector3::CreateAxisX() * lineLength);
        const auto gizmoEndAxisY = calculateGizmoAxis(-AZ::Vector3::CreateAxisY() * lineLength);
        const auto gizmoEndAxisZ = calculateGizmoAxis(-AZ::Vector3::CreateAxisZ() * lineLength);

        const AZ::Vector2 gizmoAxisX = gizmoEndAxisX - gizmoStart;
        const AZ::Vector2 gizmoAxisY = gizmoEndAxisY - gizmoStart;
        const AZ::Vector2 gizmoAxisZ = gizmoEndAxisZ - gizmoStart;

        // draw the axes of the gizmo
        debugDisplay.SetLineWidth(ed_viewportGizmoAxisLineWidth);
        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisX, 1.0f);
        debugDisplay.SetColor(AZ::Colors::Lime);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisY, 1.0f);
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisZ, 1.0f);
        debugDisplay.SetLineWidth(1.0f);

        const float labelOffset = ed_viewportGizmoAxisLabelOffset;
        const auto viewportSize = AzFramework::Vector2FromScreenSize(editorCameraState.m_viewportSize);
        const auto labelXScreenPosition = (gizmoStart + (gizmoAxisX * labelOffset)) * viewportSize;
        const auto labelYScreenPosition = (gizmoStart + (gizmoAxisY * labelOffset)) * viewportSize;
        const auto labelZScreenPosition = (gizmoStart + (gizmoAxisZ * labelOffset)) * viewportSize;

        // draw the label of of each axis for the gizmo
        const float labelSize = ed_viewportGizmoAxisLabelSize;
        debugDisplay.SetColor(AZ::Colors::White);
        debugDisplay.Draw2dTextLabel(labelXScreenPosition.GetX(), labelXScreenPosition.GetY(), labelSize, "X", true);
        debugDisplay.Draw2dTextLabel(labelYScreenPosition.GetX(), labelYScreenPosition.GetY(), labelSize, "Y", true);
        debugDisplay.Draw2dTextLabel(labelZScreenPosition.GetX(), labelZScreenPosition.GetY(), labelSize, "Z", true);
    }

    void EditorTransformComponentSelection::DisplayViewportSelection2d(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // debug draw manipulator position (when one exists) in screen space
        if (ed_viewportDisplayManipulatorPosition)
        {
            const int viewportId = viewportInfo.m_viewportId;
            const AzFramework::CameraState editorCameraState = GetCameraState(viewportId);
            const auto viewportSize = AzFramework::Vector2FromScreenSize(editorCameraState.m_viewportSize);

            AZStd::string manipulatorPosition = "none";
            if (const auto manipulatorTransform = GetManipulatorTransform(); manipulatorTransform.has_value())
            {
                AZStd::to_string(manipulatorPosition, manipulatorTransform->GetTranslation());
            }

            debugDisplay.SetColor(AZ::Colors::White);
            debugDisplay.Draw2dTextLabel(
                15.0f, viewportSize.GetY() - 25.0f, 0.6f,
                AZStd::string::format("Manipulator World Position: %s", manipulatorPosition.c_str()).c_str());
        }

        DrawAxisGizmo(viewportInfo, debugDisplay);

        m_boxSelect.Display2d(viewportInfo, debugDisplay);

        m_editorHelpers->Display2d(viewportInfo, debugDisplay);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // check what the 'authoritative' selected entity ids are after an undo/redo
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        RefreshSelectedEntityIds(selectedEntityIds);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds(const EntityIdList& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        for (const AZ::EntityId& entityId : selectedEntityIds)
        {
            AZ::TransformNotificationBus::MultiHandler::BusConnect(entityId);
        }

        // update selected entityId set
        m_selectedEntityIds.clear();
        m_selectedEntityIds.reserve(selectedEntityIds.size());
        AZStd::copy(selectedEntityIds.begin(), selectedEntityIds.end(), AZStd::inserter(m_selectedEntityIds, m_selectedEntityIds.end()));
    }

    void EditorTransformComponentSelection::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& localTM, [[maybe_unused]] const AZ::Transform& worldTM)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_transformChangedInternally)
        {
            RefreshManipulators(RefreshType::All);
        }
    }

    void EditorTransformComponentSelection::OnViewportViewEntityChanged(const AZ::EntityId& viewEntityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // if a viewport view entity has been set (e.g. we have set EditorCameraComponent to
        // match the editor camera translation/orientation), record the entity id if we have
        // a manipulator tracking it (entity id exists in m_entityIdManipulator lookups)
        // and remove it when recreating manipulators (see InitializeManipulators)
        if (viewEntityId.IsValid())
        {
            const auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(viewEntityId);
            if (entityIdLookupIt != m_entityIdManipulators.m_lookups.end())
            {
                m_editorCameraComponentEntityId = viewEntityId;
                RegenerateManipulators();
            }
        }
        else
        {
            if (m_editorCameraComponentEntityId.IsValid())
            {
                const auto editorCameraEntityId = m_editorCameraComponentEntityId;
                m_editorCameraComponentEntityId.SetInvalid();

                if (m_entityIdManipulators.m_manipulators)
                {
                    if (IsEntitySelectedInternal(editorCameraEntityId, m_selectedEntityIds))
                    {
                        RegenerateManipulators();
                    }
                }
            }
        }
    }

    void EditorTransformComponentSelection::CheckDirtyEntityIds()
    {
        if (m_selectedEntityIdsAndManipulatorsDirty)
        {
            RefreshSelectedEntityIdsAndRegenerateManipulators();
            m_selectedEntityIdsAndManipulatorsDirty = false;
        }
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIdsAndRegenerateManipulators()
    {
        RefreshSelectedEntityIds();
        RegenerateManipulators();
    }

    void EditorTransformComponentSelection::OnEntityVisibilityChanged([[maybe_unused]] const bool visibility)
    {
        m_selectedEntityIdsAndManipulatorsDirty = true;
    }

    void EditorTransformComponentSelection::OnEntityLockChanged([[maybe_unused]] const bool locked)
    {
        m_selectedEntityIdsAndManipulatorsDirty = true;
    }

    void EditorTransformComponentSelection::OnEditorModeActivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode)
    {
        switch (mode)
        {
        case ViewportEditorMode::Component:
            {
                SetAllViewportUiVisible(false);

                EditorEntityLockComponentNotificationBus::Router::BusRouterDisconnect();
                EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
                ToolsApplicationNotificationBus::Handler::BusDisconnect();
            }
            break;
        case ViewportEditorMode::Focus:
        case ViewportEditorMode::Default:
        case ViewportEditorMode::Pick:
            // noop
            break;
        }
    }

    void EditorTransformComponentSelection::OnEditorModeDeactivated(
        [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, const ViewportEditorMode mode)
    {
        switch (mode)
        {
        case ViewportEditorMode::Component:
            {
                SetAllViewportUiVisible(true);

                ToolsApplicationNotificationBus::Handler::BusConnect();
                EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
                EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
            }
            break;
        case ViewportEditorMode::Focus:
        case ViewportEditorMode::Default:
        case ViewportEditorMode::Pick:
            // noop
            break;
        }
    }

    void EditorTransformComponentSelection::CreateEntityManipulatorDeselectCommand(ScopedUndoBatch& undoBatch)
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), ManipulatorUndoRedoName);

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    AZStd::optional<AZ::Transform> EditorTransformComponentSelection::GetManipulatorTransform()
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            return { m_entityIdManipulators.m_manipulators->GetLocalTransform() };
        }

        return {};
    }

    void EditorTransformComponentSelection::SetEntityWorldTranslation(const AZ::EntityId entityId, const AZ::Vector3& worldTranslation)
    {
        Etcs::SetEntityWorldTranslation(entityId, worldTranslation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalTranslation(const AZ::EntityId entityId, const AZ::Vector3& localTranslation)
    {
        Etcs::SetEntityLocalTranslation(entityId, localTranslation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityWorldTransform(const AZ::EntityId entityId, const AZ::Transform& worldTransform)
    {
        Etcs::SetEntityWorldTransform(entityId, worldTransform, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalScale(const AZ::EntityId entityId, const float localScale)
    {
        Etcs::SetEntityLocalScale(entityId, localScale, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalRotation(const AZ::EntityId entityId, const AZ::Vector3& localRotation)
    {
        Etcs::SetEntityLocalRotation(entityId, localRotation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalRotation(AZ::EntityId entityId, const AZ::Quaternion& localRotation)
    {
        Etcs::SetEntityLocalRotation(entityId, localRotation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::OnStartPlayInEditor()
    {
        SetAllViewportUiVisible(false);
        // this is called seperately because the above method disables the switcher in component mode
        if (m_componentModeSwitcher != nullptr)
        {
            SetViewportUiSwitcherVisible(m_componentModeSwitcher->GetSwitcherId(), false);
        }
    }

    void EditorTransformComponentSelection::OnStopPlayInEditor()
    {
        SetAllViewportUiVisible(true);

        if (m_componentModeSwitcher != nullptr)
        {
            SetViewportUiSwitcherVisible(m_componentModeSwitcher->GetSwitcherId(), true);
        }
    }

    void EditorTransformComponentSelection::OnGridSnappingChanged([[maybe_unused]] const bool enabled)
    {
        m_snappingCluster.TrySetVisible(m_viewportUiVisible && !m_selectedEntityIds.empty());
    }

    void EditorTransformComponentSelection::OnReadOnlyEntityStatusChanged(const AZ::EntityId& entityId, [[maybe_unused]] bool readOnly)
    {
        if (IsEntitySelected(entityId))
        {
            RefreshSelectedEntityIdsAndRegenerateManipulators();
        }
    }

    namespace Etcs
    {
        // little raii wrapper to switch a value from true to false and back
        class ScopeSwitch
        {
        public:
            ScopeSwitch(bool& option)
                : m_option(option)
            {
                m_option = true;
            }

            ~ScopeSwitch()
            {
                m_option = false;
            }

        private:
            bool& m_option;
        };

        void SetEntityWorldTranslation(AZ::EntityId entityId, const AZ::Vector3& worldTranslation, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTranslation, worldTranslation);
        }

        void SetEntityLocalTranslation(AZ::EntityId entityId, const AZ::Vector3& localTranslation, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalTranslation, localTranslation);
        }

        void SetEntityWorldTransform(AZ::EntityId entityId, const AZ::Transform& worldTransform, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, worldTransform);
        }

        void SetEntityLocalScale(AZ::EntityId entityId, float localScale, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalUniformScale, localScale);
        }

        void SetEntityLocalRotation(AZ::EntityId entityId, const AZ::Vector3& localRotation, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalRotation, localRotation);
        }

        void SetEntityLocalRotation(AZ::EntityId entityId, const AZ::Quaternion& localRotation, bool& internal)
        {
            ScopeSwitch sw(internal);
            AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetLocalRotationQuaternion, localRotation);
        }
    } // namespace Etcs

    // explicit instantiations
    template Etcs::PivotOrientationResult Etcs::CalculatePivotOrientationForEntityIds<EntityIdManipulatorLookups>(
        const EntityIdManipulatorLookups&, ReferenceFrame);
    template Etcs::PivotOrientationResult Etcs::CalculateSelectionPivotOrientation<EntityIdManipulatorLookups>(
        const EntityIdManipulatorLookups&, const OptionalFrame&, const ReferenceFrame referenceFrame);
} // namespace AzToolsFramework
