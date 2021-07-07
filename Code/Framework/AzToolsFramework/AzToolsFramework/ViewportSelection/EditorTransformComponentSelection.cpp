/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorTransformComponentSelection.h"

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/std/algorithm.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/Commands/EntityManipulatorCommand.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>
#include <Entity/EditorEntityContextBus.h>
#include <Entity/EditorEntityHelpers.h>
#include <QApplication>
#include <QRect>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorTransformComponentSelection, AZ::SystemAllocator, 0)

    AZ_CVAR(
        float,
        cl_viewportGizmoAxisLineWidth,
        4.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The width of the line for the viewport axis gizmo");
    AZ_CVAR(
        float,
        cl_viewportGizmoAxisLineLength,
        0.7f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The length of the line for the viewport axis gizmo");
    AZ_CVAR(
        float,
        cl_viewportGizmoAxisLabelOffset,
        1.15f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The offset of the label for the viewport axis gizmo");
    AZ_CVAR(
        float,
        cl_viewportGizmoAxisLabelSize,
        1.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The size of each label for the viewport axis gizmo");
    AZ_CVAR(
        AZ::Vector2,
        cl_viewportGizmoAxisScreenPosition,
        AZ::Vector2(0.045f, 0.9f),
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The screen position of the gizmo in normalized (0-1) ndc space");

    // strings related to new viewport interaction model (EditorTransformComponentSelection)
    static const char* const s_togglePivotTitleRightClick = "Toggle pivot";
    static const char* const s_togglePivotTitleEditMenu = "Toggle Pivot Location";
    static const char* const s_togglePivotDesc = "Toggle pivot location";
    static const char* const s_manipulatorUndoRedoName = "Manipulator Adjustment";
    static const char* const s_lockSelectionTitle = "Lock Selection";
    static const char* const s_lockSelectionDesc = "Lock the selected entities so that they can't be selected in the viewport";
    static const char* const s_hideSelectionTitle = "Hide Selection";
    static const char* const s_hideSelectionDesc = "Hide the selected entities so that they don't appear in the viewport";
    static const char* const s_unlockAllTitle = "Unlock All Entities";
    static const char* const s_unlockAllDesc = "Unlock all entities the level";
    static const char* const s_showAllTitle = "Show All";
    static const char* const s_showAllDesc = "Show all entities so that they appear in the viewport";
    static const char* const s_selectAllTitle = "Select All";
    static const char* const s_selectAllDesc = "Select all entities";
    static const char* const s_invertSelectionTitle = "Invert Selection";
    static const char* const s_invertSelectionDesc = "Invert the current entity selection";
    static const char* const s_duplicateTitle = "Duplicate";
    static const char* const s_duplicateDesc = "Duplicate selected entities";
    static const char* const s_deleteTitle = "Delete";
    static const char* const s_deleteDesc = "Delete selected entities";
    static const char* const s_resetEntityTransformTitle = "Reset Entity Transform";
    static const char* const s_resetEntityTransformDesc = "Reset transform based on manipulator mode";
    static const char* const s_resetManipulatorTitle = "Reset Manipulator";
    static const char* const s_resetManipulatorDesc = "Reset the manipulator to recenter it on the selected entity";
    static const char* const s_resetTransformLocalTitle = "Reset Transform (Local)";
    static const char* const s_resetTransformLocalDesc = "Reset transform to local space";
    static const char* const s_resetTransformWorldTitle = "Reset Transform (World)";
    static const char* const s_resetTransformWorldDesc = "Reset transform to world space";

    static const char* const s_entityBoxSelectUndoRedoDesc = "Box Select Entities";
    static const char* const s_entityDeselectUndoRedoDesc = "Deselect Entity";
    static const char* const s_entitiesDeselectUndoRedoDesc = "Deselect Entities";
    static const char* const s_entitySelectUndoRedoDesc = "Select Entity";
    static const char* const s_dittoManipulatorUndoRedoDesc = "Ditto Manipulator";
    static const char* const s_resetManipulatorTranslationUndoRedoDesc = "Reset Manipulator Translation";
    static const char* const s_resetManipulatorOrientationUndoRedoDesc = "Reset Manipulator Orientation";
    static const char* const s_dittoEntityOrientationIndividualUndoRedoDesc = "Ditto orientation individual";
    static const char* const s_dittoEntityOrientationGroupUndoRedoDesc = "Ditto orientation group";
    static const char* const s_resetTranslationToParentUndoRedoDesc = "Reset translation to parent";
    static const char* const s_resetOrientationToParentUndoRedoDesc = "Reset orientation to parent";
    static const char* const s_dittoTranslationGroupUndoRedoDesc = "Ditto translation group";
    static const char* const s_dittoTranslationIndividualUndoRedoDesc = "Ditto translation individual";
    static const char* const s_dittoScaleIndividualWorldUndoRedoDesc = "Ditto scale individual world";
    static const char* const s_dittoScaleIndividualLocalUndoRedoDesc = "Ditto scale individual local";
    static const char* const s_showAllEntitiesUndoRedoDesc = s_showAllTitle;
    static const char* const s_lockSelectionUndoRedoDesc = s_lockSelectionTitle;
    static const char* const s_hideSelectionUndoRedoDesc = s_hideSelectionTitle;
    static const char* const s_unlockAllUndoRedoDesc = s_unlockAllTitle;
    static const char* const s_selectAllEntitiesUndoRedoDesc = s_selectAllTitle;
    static const char* const s_invertSelectionUndoRedoDesc = s_invertSelectionTitle;
    static const char* const s_duplicateUndoRedoDesc = s_duplicateTitle;
    static const char* const s_deleteUndoRedoDesc = s_deleteTitle;

    static const AZ::Color s_fadedXAxisColor = AZ::Color(AZ::u8(200), AZ::u8(127), AZ::u8(127), AZ::u8(255));
    static const AZ::Color s_fadedYAxisColor = AZ::Color(AZ::u8(127), AZ::u8(190), AZ::u8(127), AZ::u8(255));
    static const AZ::Color s_fadedZAxisColor = AZ::Color(AZ::u8(120), AZ::u8(120), AZ::u8(180), AZ::u8(255));

    static const AZ::Color s_pickedOrientationColor = AZ::Color(0.0f, 1.0f, 0.0f, 1.0f);
    static const AZ::Color s_selectedEntityAabbColor = AZ::Color(0.6f, 0.6f, 0.6f, 0.4f);

    static const int s_defaultViewportId = 0;

    static const float s_pivotSize = 0.075f; // the size of the pivot (box) to render when selected

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

    bool OptionalFrame::HasEntityOverride() const
    {
        return m_pickedEntityIdOverride.IsValid();
    }

    bool OptionalFrame::PickedTranslation() const
    {
        return (m_pickTypes & PickType::Translation) != 0;
    }

    bool OptionalFrame::PickedOrientation() const
    {
        return (m_pickTypes & PickType::Orientation) != 0;
    }

    void OptionalFrame::Reset()
    {
        m_orientationOverride.reset();
        m_translationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes = PickType::None;
    }

    void OptionalFrame::ResetPickedTranslation()
    {
        m_translationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes &= ~PickType::Translation;
    }

    void OptionalFrame::ResetPickedOrientation()
    {
        m_orientationOverride.reset();
        m_pickedEntityIdOverride.SetInvalid();
        m_pickTypes &= ~PickType::Orientation;
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
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
                !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
        }

        static bool IndividualDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
        }

        static bool SnapTerrain(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() ||
                 mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl());
        }

        static bool ManipulatorDitto(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
                mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
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
                !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt();
        }
    } // namespace Input

    static void DestroyManipulators(EntityIdManipulators& manipulators)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        return AZStd::vector<AZ::EntityId>(entityIdContainer.begin(), entityIdContainer.end());
    }

    // take a generic, associative container and return a vector
    template<typename EntityIdMap>
    static AZStd::vector<AZ::EntityId> EntityIdVectorFromMap(const EntityIdMap& entityIdMap)
    {
        static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        const EditorVisibleEntityDataCache& entityDataCache,
        const int viewportId,
        const ViewportInteraction::KeyboardModifiers currentKeyboardModifiers,
        const ViewportInteraction::KeyboardModifiers& previousKeyboardModifiers)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (boxSelect)
        {
            // modifier has changed - swapped from additive to subtractive box select (or vice versa)
            if (previousKeyboardModifiers != currentKeyboardModifiers)
            {
                for (AZ::EntityId entityId : potentialDeselectedEntityIds)
                {
                    editorTransformComponentSelection.AddEntityToSelection(entityId);
                }
                potentialDeselectedEntityIds.clear();

                for (AZ::EntityId entityId : potentialSelectedEntityIds)
                {
                    editorTransformComponentSelection.RemoveEntityFromSelection(entityId);
                }
                potentialSelectedEntityIds.clear();
            }

            // set the widget context before calls to ViewportWorldToScreen so we are not
            // going to constantly be pushing/popping the widget context
            ViewportInteraction::WidgetContextGuard widgetContextGuard(viewportId);

            for (size_t entityCacheIndex = 0; entityCacheIndex < entityDataCache.VisibleEntityDataCount(); ++entityCacheIndex)
            {
                if (entityDataCache.IsVisibleEntityLocked(entityCacheIndex) || !entityDataCache.IsVisibleEntityVisible(entityCacheIndex))
                {
                    continue;
                }

                const AZ::EntityId entityId = entityDataCache.GetVisibleEntityId(entityCacheIndex);
                const AZ::Vector3& entityPosition = entityDataCache.GetVisibleEntityPosition(entityCacheIndex);

                const AzFramework::ScreenPoint screenPosition = GetScreenPosition(viewportId, entityPosition);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

    // return either center or entity pivot
    static AZ::Vector3 CalculatePivotTranslation(const AZ::EntityId entityId, const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        return worldFromLocal.TransformPoint(CalculateCenterOffset(entityId, pivot));
    }

    void EditorTransformComponentSelection::SetAllViewportUiVisible(const bool visible)
    {
        SetViewportUiClusterVisible(m_transformModeClusterId, visible);
        SetViewportUiClusterVisible(m_spaceCluster.m_spaceClusterId, visible);
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
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_spaceCluster.m_spaceClusterId, buttonIdFromFrameFn(referenceFrame));
    }

    namespace ETCS
    {
        PivotOrientationResult CalculatePivotOrientation(const AZ::EntityId entityId, const ReferenceFrame referenceFrame)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
    } // namespace ETCS

    // return parent space from selection - if entity(s) share a common parent,
    // return that space, otherwise return world space
    template<typename EntityIdMapIterator>
    static ETCS::PivotOrientationResult CalculateParentSpace(EntityIdMapIterator begin, EntityIdMapIterator end)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // initialize to world with no parent
        ETCS::PivotOrientationResult result{ AZ::Quaternion::CreateIdentity(), AZ::EntityId() };

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

        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

    namespace ETCS
    {
        template<typename EntityIdMap>
        PivotOrientationResult CalculatePivotOrientationForEntityIds(const EntityIdMap& entityIdMap, const ReferenceFrame referenceFrame)
        {
            static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");

            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            // simple case with one entity
            if (entityIdMap.size() == 1)
            {
                return ETCS::CalculatePivotOrientation(entityIdMap.begin()->first, referenceFrame);
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
    } // namespace ETCS

    namespace ETCS
    {
        template<typename EntityIdMap>
        PivotOrientationResult CalculateSelectionPivotOrientation(
            const EntityIdMap& entityIdMap, const OptionalFrame& pivotOverrideFrame, const ReferenceFrame referenceFrame)
        {
            static_assert(AZStd::is_same<typename EntityIdMap::key_type, AZ::EntityId>::value, "Container key type is not an EntityId");
            static_assert(
                AZStd::is_same<typename EntityIdMap::mapped_type, EntityIdManipulatorLookup>::value,
                "Container value type is not an EntityIdManipulators::Lookup");

            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            // start - calculate orientation without considering current overrides/modifications
            PivotOrientationResult pivot = CalculatePivotOrientationForEntityIds(entityIdMap, referenceFrame);

            // if there is already an orientation override
            if (pivotOverrideFrame.HasOrientationOverride())
            {
                switch (referenceFrame)
                {
                case ReferenceFrame::Local:
                    // if we have a group selection, always use the pivot override if one
                    // is set when moving to local space (can't pick individual local space)
                    if (entityIdMap.size() > 1)
                    {
                        pivot.m_worldOrientation = pivotOverrideFrame.m_orientationOverride.value();
                    }
                    break;
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
    } // namespace ETCS

    template<typename EntityIdMap>
    static AZ::Vector3 RecalculateAverageManipulatorTranslation(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        return pivotOverrideFrame.m_translationOverride.value_or(CalculatePivotTranslationForEntityIds(entityIdMap, pivot));
    }

    template<typename EntityIdMap>
    static AZ::Quaternion RecalculateAverageManipulatorOrientation(
        const EntityIdMap& entityIdMap, const OptionalFrame& pivotOverrideFrame, const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        return ETCS::CalculateSelectionPivotOrientation(entityIdMap, pivotOverrideFrame, referenceFrame).m_worldOrientation;
    }

    template<typename EntityIdMap>
    static AZ::Transform RecalculateAverageManipulatorTransform(
        const EntityIdMap& entityIdMap,
        const OptionalFrame& pivotOverrideFrame,
        const EditorTransformComponentSelectionRequests::Pivot pivot,
        const ReferenceFrame referenceFrame)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

    static void UpdateInitialRotation(EntityIdManipulators& entityManipulators)
    {
        // save new start orientation (if moving rotation axes separate from object
        // or switching type of rotation (modifier keys change))
        for (auto& entityIdLookup : entityManipulators.m_lookups)
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

            entityIdLookup.second.m_initial = worldFromLocal;
        }
    }

    // utility function to immediately return the current reference frame
    // based on the state of the modifiers
    static ReferenceFrame ReferenceFrameFromModifiers(const ViewportInteraction::KeyboardModifiers modifiers)
    {
        if (modifiers.Shift() && !modifiers.Alt())
        {
            return ReferenceFrame::World;
        }
        else if (modifiers.Alt() && !modifiers.Shift())
        {
            return ReferenceFrame::Local;
        }
        else
        {
            return ReferenceFrame::Parent;
        }
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

            // note: used for parent and world depending on the current reference frame
            const auto pivotOrientation =
                ETCS::CalculateSelectionPivotOrientation(entityIdManipulators.m_lookups, pivotOverrideFrame, referenceFrame);

            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (AZ::EntityId entityId : entityIdContainer)
            {
                const auto entityItLookupIt = entityIdManipulators.m_lookups.find(entityId);
                if (entityItLookupIt == entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Vector3 worldTranslation = GetWorldTranslation(entityId);

                switch (referenceFrame)
                {
                case ReferenceFrame::Local:
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

                        ETCS::SetEntityWorldTranslation(
                            entityId, entityItLookupIt->second.m_initial.GetTranslation() + localOffset, transformChangedInternally);
                    }
                    break;
                case ReferenceFrame::Parent:
                case ReferenceFrame::World:
                    {
                        AZ::Quaternion offsetRotation = pivotOrientation.m_worldOrientation *
                            QuaternionFromTransformNoScaling(entityIdManipulators.m_manipulators->GetLocalTransform().GetInverse());

                        const AZ::Vector3 localOffset = offsetRotation.TransformVector(action.LocalPositionOffset());

                        if (action.m_modifiers != prevModifiers)
                        {
                            entityItLookupIt->second.m_initial = AZ::Transform::CreateTranslation(worldTranslation - localOffset);
                        }

                        ETCS::SetEntityWorldTranslation(
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

    static void HandleAccents(
        const bool hasSelectedEntities,
        const AZ::EntityId entityIdUnderCursor,
        const bool ctrlHeld,
        AZ::EntityId& hoveredEntityId,
        const ViewportInteraction::MouseButtons mouseButtons,
        const bool usingBoxSelect)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const bool invalidMouseButtonHeld = mouseButtons.Middle() || mouseButtons.Right();

        if ((hoveredEntityId.IsValid() && hoveredEntityId != entityIdUnderCursor) ||
            (hasSelectedEntities && !ctrlHeld && hoveredEntityId.IsValid()) || invalidMouseButtonHeld)
        {
            if (hoveredEntityId.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, hoveredEntityId, false);

                hoveredEntityId.SetInvalid();
            }
        }

        if (!invalidMouseButtonHeld && !usingBoxSelect && (!hasSelectedEntities || ctrlHeld))
        {
            if (entityIdUnderCursor.IsValid())
            {
                ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetEntityHighlighted, entityIdUnderCursor, true);

                hoveredEntityId = entityIdUnderCursor;
            }
        }
    }

    static AZ::Vector3 PickTerrainPosition(const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const int viewportId = mouseInteraction.m_interactionId.m_viewportId;
        // get unsnapped terrain position (world space)
        AZ::Vector3 worldSurfacePosition;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            worldSurfacePosition, viewportId, &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
            mouseInteraction.m_mousePick.m_screenCoordinates);

        // convert to local space - snap if enabled
        const GridSnapParameters gridSnapParams = GridSnapSettings(viewportId);
        const AZ::Vector3 finalSurfacePosition = gridSnapParams.m_gridSnap
            ? CalculateSnappedTerrainPosition(worldSurfacePosition, AZ::Transform::CreateIdentity(), viewportId, gridSnapParams.m_gridSize)
            : worldSurfacePosition;

        return finalSurfacePosition;
    }

    // is the passed entity id contained with in the entity id list
    template<typename EntityIdContainer>
    static bool IsEntitySelectedInternal(AZ::EntityId entityId, const EntityIdContainer& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        const auto entityIdIt = selectedEntityIds.find(entityId);
        return entityIdIt != selectedEntityIds.end();
    }

    static EntityIdTransformMap RecordTransformsBefore(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
    static bool SelectableInVisibleViewportCache(const EditorVisibleEntityDataCache& entityDataCache, const AZ::EntityId entityId)
    {
        if (auto entityIndex = entityDataCache.GetVisibleEntityIndexFromId(entityId))
        {
            return entityDataCache.IsVisibleEntitySelectableInViewport(*entityIndex);
        }

        return false;
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

    EditorTransformComponentSelection::EditorTransformComponentSelection(const EditorVisibleEntityDataCache* entityDataCache)
        : m_entityDataCache(entityDataCache)
    {
        const AzFramework::EntityContextId entityContextId = GetEntityContextId();

        m_editorHelpers = AZStd::make_unique<EditorHelpers>(entityDataCache);

        EditorEventsBus::Handler::BusConnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusConnect(entityContextId);
        ToolsApplicationNotificationBus::Handler::BusConnect();
        Camera::EditorCameraNotificationBus::Handler::BusConnect();
        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusConnect(entityContextId);
        EditorEntityContextNotificationBus::Handler::BusConnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusConnect(entityContextId);
        EditorContextMenuBus::Handler::BusConnect();

        CreateTransformModeSelectionCluster();
        CreateSpaceSelectionCluster();
        RegisterActions();
        SetupBoxSelect();
        RefreshSelectedEntityIdsAndRegenerateManipulators();
    }

    EditorTransformComponentSelection::~EditorTransformComponentSelection()
    {
        m_selectedEntityIds.clear();
        DestroyManipulators(m_entityIdManipulators);

        DestroyCluster(m_transformModeClusterId);
        DestroyCluster(m_spaceCluster.m_spaceClusterId);

        UnregisterActions();

        m_pivotOverrideFrame.Reset();

        EditorContextMenuBus::Handler::BusConnect();
        EditorManipulatorCommandUndoRedoRequestBus::Handler::BusDisconnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterDisconnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        ComponentModeFramework::EditorComponentModeNotificationBus::Handler::BusDisconnect();
        Camera::EditorCameraNotificationBus::Handler::BusDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorTransformComponentSelectionRequestBus::Handler::BusDisconnect();
        EditorEventsBus::Handler::BusDisconnect();
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
                    AZStd::make_unique<SelectionCommand>(EntityIdList(), s_entityBoxSelectUndoRedoDesc);
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
            [this, entityBoxSelectData]()
            {
                entityBoxSelectData->m_boxSelectSelectionCommand->UpdateSelection(EntityIdVectorFromContainer(m_selectedEntityIds));

                // if we know a change in selection has occurred, record the undo step
                if (!entityBoxSelectData->m_potentialDeselectedEntityIds.empty() ||
                    !entityBoxSelectData->m_potentialSelectedEntityIds.empty())
                {
                    ScopedUndoBatch undoBatch(s_entityBoxSelectUndoRedoDesc);

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
                const auto modifiers = ViewportInteraction::KeyboardModifiers(
                    ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));

                if (m_boxSelect.PreviousModifiers() != modifiers)
                {
                    EntityBoxSelectUpdateGeneral(
                        m_boxSelect.BoxRegion(), *this, m_selectedEntityIds, entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect,
                        entityBoxSelectData->m_potentialSelectedEntityIds, entityBoxSelectData->m_potentialDeselectedEntityIds,
                        *m_entityDataCache, viewportInfo.m_viewportId, modifiers, m_boxSelect.PreviousModifiers());
                }

                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(s_selectedEntityAabbColor);

                for (AZ::EntityId entityId : entityBoxSelectData->m_potentialSelectedEntityIds)
                {
                    const auto entityIdIt = entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.find(entityId);

                    // don't show box when re-adding from previous selection
                    if (entityIdIt != entityBoxSelectData->m_selectedEntityIdsBeforeBoxSelect.end())
                    {
                        continue;
                    }

                    const AZ::Aabb bound = CalculateEditorEntitySelectionBounds(entityId, viewportInfo);
                    debugDisplay.DrawSolidBox(bound.GetMin(), bound.GetMax());
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
                 TransformNormalizedScale(m_entityIdManipulators.m_manipulators->GetLocalTransform()),
                 m_pivotOverrideFrame.m_pickedEntityIdOverride };
    }

    void EditorTransformComponentSelection::BeginRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // we must have an existing parent undo batch active when beginning to record
        // a manipulator command
        UndoSystem::URSequencePoint* currentUndoOperation = nullptr;
        ToolsApplicationRequests::Bus::BroadcastResult(currentUndoOperation, &ToolsApplicationRequests::GetCurrentUndoBatch);

        if (currentUndoOperation)
        {
            // check here if translation or orientation override are set
            m_manipulatorMoveCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);
        }
    }

    void EditorTransformComponentSelection::EndRecordManipulatorCommand()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<TranslationManipulators> translationManipulators = AZStd::make_unique<TranslationManipulators>(
            TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne());

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

                // [ref 1.]
                BeginRecordManipulatorCommand();
            });

        ViewportInteraction::KeyboardModifiers prevModifiers{};
        translationManipulators->InstallLinearManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const LinearManipulator::Action& action) mutable -> void
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

                // [ref 1.]
                BeginRecordManipulatorCommand();
            });

        translationManipulators->InstallPlanarManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const PlanarManipulator::Action& action) mutable -> void
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

                // [ref 1.]
                BeginRecordManipulatorCommand();
            });

        translationManipulators->InstallSurfaceManipulatorMouseMoveCallback(
            [this, prevModifiers, manipulatorEntityIds](const SurfaceManipulator::Action& action) mutable -> void
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

        // transfer ownership
        m_entityIdManipulators.m_manipulators = AZStd::move(translationManipulators);
    }

    void EditorTransformComponentSelection::CreateRotationManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<RotationManipulators> rotationManipulators =
            AZStd::make_unique<RotationManipulators>(AZ::Transform::CreateIdentity());

        InitializeManipulators(*rotationManipulators);

        rotationManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        // view
        rotationManipulators->SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            2.0f, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
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
            [this, sharedRotationState]([[maybe_unused]] const AngularManipulator::Action& action) mutable -> void
            {
                sharedRotationState->m_savedOrientation = AZ::Quaternion::CreateIdentity();
                sharedRotationState->m_referenceFrameAtMouseDown = m_referenceFrame;
                // important to sort entityIds based on hierarchy order when updating transforms
                BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, sharedRotationState->m_entityIds);

                for (auto& entityIdLookup : m_entityIdManipulators.m_lookups)
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

                    entityIdLookup.second.m_initial = worldFromLocal;
                }

                m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
                m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());

                // [ref 1.]
                BeginRecordManipulatorCommand();
            });

        ViewportInteraction::KeyboardModifiers prevModifiers{};
        rotationManipulators->InstallMouseMoveCallback(
            [this, prevModifiers, sharedRotationState](const AngularManipulator::Action& action) mutable -> void
            {
                const ReferenceFrame referenceFrame = m_spaceCluster.m_spaceLock.value_or(ReferenceFrameFromModifiers(action.m_modifiers));
                const AZ::Quaternion manipulatorOrientation = action.m_start.m_rotation * action.m_current.m_delta;
                // store the pivot override frame when positioning the manipulator manually (ctrl)
                // so we don't lose the orientation when adding/removing entities from the selection
                if (action.m_modifiers.Ctrl())
                {
                    m_pivotOverrideFrame.m_orientationOverride = manipulatorOrientation;
                }

                // only update the manipulator orientation if we're rotating in a local reference frame or we're
                // manually modifying the manipulator orientation independent of the entity by holding ctrl
                if ((sharedRotationState->m_referenceFrameAtMouseDown == ReferenceFrame::Local &&
                     m_entityIdManipulators.m_lookups.size() == 1) ||
                    action.m_modifiers.Ctrl())
                {
                    m_entityIdManipulators.m_manipulators->SetLocalTransform(AZ::Transform::CreateFromQuaternionAndTranslation(
                        manipulatorOrientation, m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));
                }

                // save state if we change the type of rotation we're doing to to prevent snapping
                if (prevModifiers != action.m_modifiers)
                {
                    UpdateInitialRotation(m_entityIdManipulators);
                    sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
                }

                // allow the user to modify the orientation without moving the object if ctrl is held
                if (action.m_modifiers.Ctrl())
                {
                    UpdateInitialRotation(m_entityIdManipulators);
                    sharedRotationState->m_savedOrientation = action.m_current.m_delta.GetInverseFull();
                }
                else
                {
                    const auto pivotOrientation = ETCS::CalculateSelectionPivotOrientation(
                        m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, ReferenceFrame::Parent);

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

                        switch (referenceFrame)
                        {
                        case ReferenceFrame::Local:
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
                        case ReferenceFrame::Parent:
                            {
                                const AZ::Transform pivotTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                                    pivotOrientation.m_worldOrientation,
                                    m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());

                                const AZ::Transform transformInPivotSpace =
                                    pivotTransform.GetInverse() * entityIdLookupIt->second.m_initial;

                                SetEntityWorldTransform(entityId, pivotTransform * offsetRotation * transformInPivotSpace);
                            }
                            break;
                        case ReferenceFrame::World:
                            {
                                const AZ::Transform pivotTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                                    AZ::Quaternion::CreateIdentity(),
                                    m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation());
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        AZStd::unique_ptr<ScaleManipulators> scaleManipulators = AZStd::make_unique<ScaleManipulators>(AZ::Transform::CreateIdentity());

        InitializeManipulators(*scaleManipulators);

        scaleManipulators->SetLocalTransform(
            RecalculateAverageManipulatorTransform(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));

        scaleManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        scaleManipulators->ConfigureView(2.0f, AZ::Color::CreateOne(), AZ::Color::CreateOne(), AZ::Color::CreateOne());

        // lambdas capture shared_ptr by value to increment ref count
        auto manipulatorEntityIds = AZStd::make_shared<ManipulatorEntityIds>();

        auto uniformLeftMouseDownCallback = [this, manipulatorEntityIds]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            // important to sort entityIds based on hierarchy order when updating transforms
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds->m_entityIds);

            for (auto& entityIdLookup : m_entityIdManipulators.m_lookups)
            {
                AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(worldFromLocal, entityIdLookup.first, &AZ::TransformBus::Events::GetWorldTM);

                entityIdLookup.second.m_initial = worldFromLocal;
            }

            m_axisPreview.m_translation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
            m_axisPreview.m_orientation = QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());
        };

        auto uniformLeftMouseUpCallback = [this, manipulatorEntityIds]([[maybe_unused]] const LinearManipulator::Action& action)
        {
            AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                manipulatorEntityIds->m_entityIds);

            m_entityIdManipulators.m_manipulators->SetLocalTransform(RecalculateAverageManipulatorTransform(
                m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_pivotMode, m_referenceFrame));
        };

        auto uniformLeftMouseMoveCallback = [this, manipulatorEntityIds](const LinearManipulator::Action& action)
        {
            // note: must use sorted entityIds based on hierarchy order when updating transforms
            for (AZ::EntityId entityId : manipulatorEntityIds->m_entityIds)
            {
                auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(entityId);
                if (entityIdLookupIt == m_entityIdManipulators.m_lookups.end())
                {
                    continue;
                }

                const AZ::Transform initial = entityIdLookupIt->second.m_initial;
                const float initialScale = initial.GetUniformScale();

                const auto sumVectorElements = [](const AZ::Vector3& vec)
                {
                    return vec.GetX() + vec.GetY() + vec.GetZ();
                };

                const float uniformScale = action.m_start.m_sign * sumVectorElements(action.LocalScaleOffset());
                const float scale = AZ::GetClamp(1.0f + uniformScale / initialScale, AZ::MinTransformScale, AZ::MaxTransformScale);
                const AZ::Transform scaleTransform = AZ::Transform::CreateUniformScale(scale);

                if (action.m_modifiers.Alt())
                {
                    const AZ::Transform pivotTransform = TransformNormalizedScale(entityIdLookupIt->second.m_initial);
                    const AZ::Transform transformInPivotSpace = pivotTransform.GetInverse() * initial;

                    SetEntityWorldTransform(entityId, pivotTransform * scaleTransform * transformInPivotSpace);
                }
                else
                {
                    const AZ::Transform pivotTransform =
                        TransformNormalizedScale(m_entityIdManipulators.m_manipulators->GetLocalTransform());
                    const AZ::Transform transformInPivotSpace = pivotTransform.GetInverse() * initial;

                    SetEntityWorldTransform(entityId, pivotTransform * scaleTransform * transformInPivotSpace);
                }
            }
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!UndoRedoOperationInProgress())
        {
            ScopedUndoBatch undoBatch(s_entitiesDeselectUndoRedoDesc);

            // restore manipulator overrides when undoing
            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // select must happen after to ensure in the undo/redo step the selection command
            // happens before the manipulator command
            auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), s_entitiesDeselectUndoRedoDesc);
            selectionCommand->SetParent(undoBatch.GetUndoBatch());
            selectionCommand.release();
        }

        m_selectedEntityIds.clear();
        SetSelectedEntities(EntityIdList());

        DestroyManipulators(m_entityIdManipulators);
        m_pivotOverrideFrame.Reset();
    }

    bool EditorTransformComponentSelection::SelectDeselect(const AZ::EntityId entityIdUnderCursor)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (entityIdUnderCursor.IsValid())
        {
            if (IsEntitySelectedInternal(entityIdUnderCursor, m_selectedEntityIds))
            {
                if (!UndoRedoOperationInProgress())
                {
                    RemoveEntityFromSelection(entityIdUnderCursor);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(s_entityDeselectUndoRedoDesc);

                    // store manipulator state when removing last entity from selection
                    if (m_entityIdManipulators.m_manipulators && nextEntityIds.empty())
                    {
                        CreateEntityManipulatorDeselectCommand(undoBatch);
                    }

                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_entityDeselectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }
            else
            {
                if (!UndoRedoOperationInProgress())
                {
                    AddEntityToSelection(entityIdUnderCursor);

                    const auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                    ScopedUndoBatch undoBatch(s_entitySelectUndoRedoDesc);
                    auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_entitySelectUndoRedoDesc);
                    selectionCommand->SetParent(undoBatch.GetUndoBatch());
                    selectionCommand.release();

                    SetSelectedEntities(nextEntityIds);
                }
            }

            return true;
        }

        return false;
    }

    bool EditorTransformComponentSelection::HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        CheckDirtyEntityIds();

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;

        const AzFramework::CameraState cameraState = GetCameraState(viewportId);

        // set the widget context before calls to ViewportWorldToScreen so we are not
        // going to constantly be pushing/popping the widget context
        ViewportInteraction::WidgetContextGuard widgetContextGuard(viewportId);

        m_cachedEntityIdUnderCursor = m_editorHelpers->HandleMouseInteraction(cameraState, mouseInteraction);

        const auto selectClickEvent = ClickDetectorEventFromViewportInteraction(mouseInteraction);
        m_cursorState.SetCurrentPosition(mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates);
        const auto clickOutcome = m_clickDetector.DetectClick(selectClickEvent, m_cursorState.CursorDelta());

        // for entities selected with no bounds of their own (just TransformComponent)
        // check selection against the selection indicator aabb
        for (AZ::EntityId entityId : m_selectedEntityIds)
        {
            if (!SelectableInVisibleViewportCache(*m_entityDataCache, entityId))
            {
                continue;
            }

            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            const AZ::Vector3 boxPosition = worldFromLocal.TransformPoint(CalculateCenterOffset(entityId, m_pivotMode));

            const AZ::Vector3 scaledSize =
                AZ::Vector3(s_pivotSize) * CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

            if (AabbIntersectMouseRay(
                    mouseInteraction.m_mouseInteraction, AZ::Aabb::CreateFromMinMax(boxPosition - scaledSize, boxPosition + scaledSize)))
            {
                m_cachedEntityIdUnderCursor = entityId;
            }
        }

        const AZ::EntityId entityIdUnderCursor = m_cachedEntityIdUnderCursor;

        EditorContextMenuUpdate(m_contextMenu, mouseInteraction);

        m_boxSelect.HandleMouseInteraction(mouseInteraction);

        if (Input::CycleManipulator(mouseInteraction))
        {
            const size_t scrollBound = 2;
            const auto nextMode =
                (static_cast<int>(m_mode) + scrollBound + (MouseWheelDelta(mouseInteraction) < 0.0f ? 1 : -1)) % scrollBound;

            SetTransformMode(static_cast<Mode>(nextMode));

            return true;
        }

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

        if (!m_selectedEntityIds.empty())
        {
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

            // group copying/alignment to specific entity - 'ditto' position/orientation for group
            if (Input::GroupDitto(mouseInteraction))
            {
                if (entityIdUnderCursor.IsValid())
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

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

                    return false;
                }
            }

            // individual copying/alignment to specific entity - 'ditto' position/orientation for individual
            if (Input::IndividualDitto(mouseInteraction))
            {
                if (entityIdUnderCursor.IsValid())
                {
                    AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

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

                    return false;
                }
            }

            // try snapping to the terrain (if in Translation mode) and entity wasn't picked
            if (Input::SnapTerrain(mouseInteraction))
            {
                for (AZ::EntityId entityId : m_selectedEntityIds)
                {
                    ScopedUndoBatch::MarkEntityDirty(entityId);
                }

                if (m_mode == Mode::Translation)
                {
                    const AZ::Vector3 finalSurfacePosition = PickTerrainPosition(mouseInteraction.m_mouseInteraction);

                    // handle modifier alternatives
                    if (Input::IndividualDitto(mouseInteraction))
                    {
                        CopyTranslationToSelectedEntitiesIndividual(finalSurfacePosition);
                    }
                    else if (Input::GroupDitto(mouseInteraction))
                    {
                        CopyTranslationToSelectedEntitiesGroup(finalSurfacePosition);
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

                return false;
            }

            // set manipulator pivot override translation or orientation (update manipulators)
            if (Input::ManipulatorDitto(mouseInteraction))
            {
                if (m_entityIdManipulators.m_manipulators)
                {
                    ScopedUndoBatch undoBatch(s_dittoManipulatorUndoRedoDesc);

                    auto manipulatorCommand =
                        AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                    if (entityIdUnderCursor.IsValid())
                    {
                        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(worldFromLocal, entityIdUnderCursor, &AZ::TransformBus::Events::GetWorldTM);

                        // set orientation/translation to match picked entity
                        switch (m_mode)
                        {
                        case Mode::Rotation:
                            OverrideManipulatorOrientation(QuaternionFromTransformNoScaling(worldFromLocal));
                            break;
                        case Mode::Translation:
                            OverrideManipulatorTranslation(worldFromLocal.GetTranslation());
                            break;
                        case Mode::Scale:
                            // do nothing
                            break;
                        default:
                            break;
                        }

                        // only update pivot override when in translation or rotation mode
                        switch (m_mode)
                        {
                        case Mode::Rotation:
                            m_pivotOverrideFrame.m_pickTypes |= OptionalFrame::PickType::Orientation;
                            [[fallthrough]];
                        case Mode::Translation:
                            m_pivotOverrideFrame.m_pickTypes |= OptionalFrame::PickType::Translation;
                            m_pivotOverrideFrame.m_pickedEntityIdOverride = entityIdUnderCursor;
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
                        // match the same behavior as if we pressed Ctrl+R to reset the manipulator
                        DelegateClearManipulatorOverride();
                    }

                    manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                        BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                        m_entityIdManipulators.m_manipulators->GetLocalTransform(), entityIdUnderCursor));

                    manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                    manipulatorCommand.release();
                }
            }

            return false;
        }

        // standard toggle selection
        if (Input::IndividualSelect(clickOutcome))
        {
            SelectDeselect(entityIdUnderCursor);
        }

        return false;
    }

    template<typename T>
    static void AddAction(
        AZStd::vector<AZStd::unique_ptr<QAction>>& actions,
        const QList<QKeySequence>& keySequences,
        int actionId,
        const QString& name,
        const QString& statusTip,
        const T& callback)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        actions.emplace_back(AZStd::make_unique<QAction>(nullptr));

        actions.back()->setShortcuts(keySequences);
        actions.back()->setText(name);
        actions.back()->setStatusTip(statusTip);

        QObject::connect(actions.back().get(), &QAction::triggered, actions.back().get(), callback);

        EditorActionRequestBus::Broadcast(&EditorActionRequests::AddActionViaBus, actionId, actions.back().get());
    }

    void EditorTransformComponentSelection::OnEscape()
    {
        DeselectEntities();
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
        case Mode::Scale:
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();
            break;
        case Mode::Translation:
            ClearManipulatorTranslationOverride();
            break;
        }
    }

    void EditorTransformComponentSelection::RegisterActions()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // note: see Code/Editor/Resource.h for ID_EDIT_<action> ids

        const auto lockUnlock = [this](const bool lock)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_lockSelectionUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);
            for (AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityLockState(entityId, lock);
            }

            RegenerateManipulators();
        };

        // lock selection
        AddAction(
            m_actions, { QKeySequence(Qt::Key_L) },
            /*ID_EDIT_FREEZE =*/32900, s_lockSelectionTitle, s_lockSelectionDesc,
            [lockUnlock]()
            {
                lockUnlock(true);
            });

        // unlock selection
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_L) },
            /*ID_EDIT_UNFREEZE =*/32973, s_lockSelectionTitle, s_lockSelectionDesc,
            [lockUnlock]()
            {
                lockUnlock(false);
            });

        const auto showHide = [this](const bool show)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

            ScopedUndoBatch undoBatch(s_hideSelectionUndoRedoDesc);

            if (m_entityIdManipulators.m_manipulators)
            {
                CreateEntityManipulatorDeselectCommand(undoBatch);
            }

            // make a copy of selected entity ids
            const auto selectedEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);
            for (AZ::EntityId entityId : selectedEntityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);
                SetEntityVisibility(entityId, show);
            }

            RegenerateManipulators();
        };

        // hide selection
        AddAction(
            m_actions, { QKeySequence(Qt::Key_H) },
            /*ID_EDIT_HIDE =*/32898, s_hideSelectionTitle, s_hideSelectionDesc,
            [showHide]()
            {
                showHide(false);
            });

        // show selection
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_H) },
            /*ID_EDIT_UNHIDE =*/32974, s_hideSelectionTitle, s_hideSelectionDesc,
            [showHide]()
            {
                showHide(true);
            });

        // unlock all entities in the level/scene
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L) },
            /*ID_EDIT_UNFREEZEALL =*/32901, s_unlockAllTitle, s_unlockAllDesc,
            []()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                ScopedUndoBatch undoBatch(s_unlockAllUndoRedoDesc);

                EnumerateEditorEntities(
                    [](AZ::EntityId entityId)
                    {
                        ScopedUndoBatch::MarkEntityDirty(entityId);
                        SetEntityLockState(entityId, false);
                    });
            });

        // show all entities in the level/scene
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H) },
            /*ID_EDIT_UNHIDEALL =*/32899, s_showAllTitle, s_showAllDesc,
            []()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                ScopedUndoBatch undoBatch(s_showAllEntitiesUndoRedoDesc);

                EnumerateEditorEntities(
                    [](AZ::EntityId entityId)
                    {
                        ScopedUndoBatch::MarkEntityDirty(entityId);
                        SetEntityVisibility(entityId, true);
                    });
            });

        // select all entities in the level/scene
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_A) },
            /*ID_EDIT_SELECTALL =*/33376, s_selectAllTitle, s_selectAllDesc,
            [this]()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                ScopedUndoBatch undoBatch(s_selectAllEntitiesUndoRedoDesc);

                if (m_entityIdManipulators.m_manipulators)
                {
                    auto manipulatorCommand =
                        AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                    // note, nothing will change that the manipulatorCommand needs to keep track
                    // for after so no need to call SetManipulatorAfter

                    manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                    manipulatorCommand.release();
                }

                EnumerateEditorEntities(
                    [this](AZ::EntityId entityId)
                    {
                        if (IsSelectableInViewport(entityId))
                        {
                            AddEntityToSelection(entityId);
                        }
                    });

                auto nextEntityIds = EntityIdVectorFromContainer(m_selectedEntityIds);

                auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_selectAllEntitiesUndoRedoDesc);
                selectionCommand->SetParent(undoBatch.GetUndoBatch());
                selectionCommand.release();

                SetSelectedEntities(nextEntityIds);
                RegenerateManipulators();
            });

        // invert current selection
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I) },
            /*ID_EDIT_INVERTSELECTION =*/33692, s_invertSelectionTitle, s_invertSelectionDesc,
            [this]()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                ScopedUndoBatch undoBatch(s_invertSelectionUndoRedoDesc);

                if (m_entityIdManipulators.m_manipulators)
                {
                    auto manipulatorCommand =
                        AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

                    // note, nothing will change that the manipulatorCommand needs to keep track
                    // for after so no need to call SetManipulatorAfter

                    manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
                    manipulatorCommand.release();
                }

                EntityIdSet entityIds;
                EnumerateEditorEntities(
                    [this, &entityIds](AZ::EntityId entityId)
                    {
                        const auto entityIdIt = AZStd::find(m_selectedEntityIds.begin(), m_selectedEntityIds.end(), entityId);
                        if (entityIdIt == m_selectedEntityIds.end())
                        {
                            if (IsSelectableInViewport(entityId))
                            {
                                entityIds.insert(entityId);
                            }
                        }
                    });

                m_selectedEntityIds = entityIds;

                auto nextEntityIds = EntityIdVectorFromContainer(entityIds);

                auto selectionCommand = AZStd::make_unique<SelectionCommand>(nextEntityIds, s_invertSelectionUndoRedoDesc);
                selectionCommand->SetParent(undoBatch.GetUndoBatch());
                selectionCommand.release();

                SetSelectedEntities(nextEntityIds);
                RegenerateManipulators();
            });

        // duplicate selection
        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_D) },
            /*ID_EDIT_CLONE =*/33525, s_duplicateTitle, s_duplicateDesc,
            []()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                // Clear Widget selection - Prevents issues caused by cloning entities while a property in the Reflected Property Editor
                // is being edited.
                if (QApplication::focusWidget())
                {
                    QApplication::focusWidget()->clearFocus();
                }

                ScopedUndoBatch undoBatch(s_duplicateUndoRedoDesc);
                auto selectionCommand = AZStd::make_unique<SelectionCommand>(EntityIdList(), s_duplicateUndoRedoDesc);
                selectionCommand->SetParent(undoBatch.GetUndoBatch());
                selectionCommand.release();

                bool handled = false;
                EditorRequestBus::Broadcast(&EditorRequests::CloneSelection, handled);

                // selection update handled in AfterEntitySelectionChanged
            });

        // delete selection
        AddAction(
            m_actions, { QKeySequence(Qt::Key_Delete) },
            /*ID_EDIT_DELETE=*/33480, s_deleteTitle, s_deleteDesc,
            [this]()
            {
                AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

                ScopedUndoBatch undoBatch(s_deleteUndoRedoDesc);

                CreateEntityManipulatorDeselectCommand(undoBatch);

                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequests::DeleteEntitiesAndAllDescendants, EntityIdVectorFromContainer(m_selectedEntityIds));

                m_selectedEntityIds.clear();
                m_pivotOverrideFrame.Reset();
            });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_Space) },
            /*ID_EDIT_ESCAPE=*/33513, "", "",
            [this]()
            {
                DeselectEntities();
            });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_P) },
            /*ID_EDIT_PIVOT=*/36203, s_togglePivotTitleEditMenu, s_togglePivotDesc,
            [this]()
            {
                ToggleCenterPivotSelection();
            });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_R) },
            /*ID_EDIT_RESET=*/36204, s_resetEntityTransformTitle, s_resetEntityTransformDesc,
            [this]()
            {
                switch (m_mode)
                {
                case Mode::Rotation:
                    ResetOrientationForSelectedEntitiesLocal();
                    break;
                case Mode::Scale:
                    CopyScaleToSelectedEntitiesIndividualLocal(1.0f);
                    break;
                case Mode::Translation:
                    ResetTranslationForSelectedEntitiesLocal();
                    break;
                }
            });

        AddAction(
            m_actions, { QKeySequence(Qt::CTRL + Qt::Key_R) },
            /*ID_EDIT_RESET_MANIPULATOR=*/36207, s_resetManipulatorTitle, s_resetManipulatorDesc,
            AZStd::bind(AZStd::mem_fn(&EditorTransformComponentSelection::DelegateClearManipulatorOverride), this));

        AddAction(
            m_actions, { QKeySequence(Qt::ALT + Qt::Key_R) },
            /*ID_EDIT_RESET_LOCAL=*/36205, s_resetTransformLocalTitle, s_resetTransformLocalDesc,
            [this]()
            {
                switch (m_mode)
                {
                case Mode::Rotation:
                    ResetOrientationForSelectedEntitiesLocal();
                    break;
                case Mode::Scale:
                    CopyScaleToSelectedEntitiesIndividualWorld(1.0f);
                    break;
                case Mode::Translation:
                    // do nothing
                    break;
                }
            });

        AddAction(
            m_actions, { QKeySequence(Qt::SHIFT + Qt::Key_R) },
            /*ID_EDIT_RESET_WORLD=*/36206, s_resetTransformWorldTitle, s_resetTransformWorldDesc,
            [this]()
            {
                switch (m_mode)
                {
                case Mode::Rotation:
                    {
                        // begin an undo batch so operations inside CopyOrientation... and
                        // DelegateClear... are grouped into a single undo/redo
                        ScopedUndoBatch undoBatch{ s_resetTransformWorldTitle };
                        CopyOrientationToSelectedEntitiesIndividual(AZ::Quaternion::CreateIdentity());
                        ClearManipulatorOrientationOverride();
                    }
                    break;
                case Mode::Scale:
                case Mode::Translation:
                    break;
                }
            });

        AddAction(
            m_actions, { QKeySequence(Qt::Key_U) },
            /*ID_VIEWPORTUI_VISIBLE=*/50040, "Toggle Viewport UI", "Hide/Show Viewport UI",
            [this]()
            {
                SetAllViewportUiVisible(!m_viewportUiVisible);
            });

        EditorMenuRequestBus::Broadcast(&EditorMenuRequests::RestoreEditMenuToDefault);
    }

    void EditorTransformComponentSelection::UnregisterActions()
    {
        for (auto& action : m_actions)
        {
            EditorActionRequestBus::Broadcast(&EditorActionRequests::RemoveActionViaBus, action.get());
        }

        m_actions.clear();
    }

    void EditorTransformComponentSelection::UnregisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Unregister();
        }
    }

    void EditorTransformComponentSelection::RegisterManipulator()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators && !m_entityIdManipulators.m_manipulators->Registered())
        {
            m_entityIdManipulators.m_manipulators->Register(g_mainManipulatorManagerId);
        }
    }

    void EditorTransformComponentSelection::CreateEntityIdManipulators()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        m_rotateButtonId = RegisterClusterButton(m_transformModeClusterId, "Translate");
        m_scaleButtonId = RegisterClusterButton(m_transformModeClusterId, "Scale");

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

    void EditorTransformComponentSelection::CreateSpaceSelectionCluster()
    {
        // create the cluster for switching spaces/reference frames
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_spaceCluster.m_spaceClusterId, ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopRight);

        // create and register the buttons (strings correspond to icons even if the values appear different)
        m_spaceCluster.m_worldButtonId = RegisterClusterButton(m_spaceCluster.m_spaceClusterId, "World");
        m_spaceCluster.m_parentButtonId = RegisterClusterButton(m_spaceCluster.m_spaceClusterId, "Parent");
        m_spaceCluster.m_localButtonId = RegisterClusterButton(m_spaceCluster.m_spaceClusterId, "Local");

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
                m_spaceCluster.m_spaceClusterId, buttonId, m_spaceCluster.m_spaceLock.has_value());
        };

        m_spaceCluster.m_spaceSelectionHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onButtonClicked);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_spaceCluster.m_spaceClusterId, m_spaceCluster.m_spaceSelectionHandler);
    }

    EditorTransformComponentSelectionRequests::Mode EditorTransformComponentSelection::GetTransformMode()
    {
        return m_mode;
    }

    void EditorTransformComponentSelection::SetTransformMode(const Mode mode)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (mode == m_mode)
        {
            return;
        }

        if (m_pivotOverrideFrame.m_orientationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_orientationOverride =
                QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform());
        }

        if (m_pivotOverrideFrame.m_translationOverride && m_entityIdManipulators.m_manipulators)
        {
            m_pivotOverrideFrame.m_translationOverride = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();
        }

        m_mode = mode;

        // set the corresponding Viewport UI button to active
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

    void EditorTransformComponentSelection::UndoRedoEntityManipulatorCommand(
        const AZ::u8 pivotOverride, const AZ::Transform& transform, const AZ::EntityId entityId)
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

            if (entityId.IsValid())
            {
                m_pivotOverrideFrame.m_pickedEntityIdOverride = entityId;
            }
        }
    }

    void EditorTransformComponentSelection::AddEntityToSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_selectedEntityIds.insert(entityId);

        AZ::TransformNotificationBus::MultiHandler::BusConnect(entityId);
    }

    void EditorTransformComponentSelection::RemoveEntityFromSelection(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_selectedEntityIds.erase(entityId);

        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(entityId);
    }

    bool EditorTransformComponentSelection::IsEntitySelected(const AZ::EntityId entityId) const
    {
        return IsEntitySelectedInternal(entityId, m_selectedEntityIds);
    }

    void EditorTransformComponentSelection::SetSelectedEntities(const EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // we are responsible for updating the current selection
        m_didSetSelectedEntities = true;
        ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, entityIds);
    }

    void EditorTransformComponentSelection::RefreshManipulators(const RefreshType refreshType)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        m_pivotOverrideFrame.m_translationOverride = translation;

        if (m_entityIdManipulators.m_manipulators)
        {
            m_entityIdManipulators.m_manipulators->SetLocalPosition(translation);
            m_entityIdManipulators.m_manipulators->SetBoundsDirty();
        }
    }

    void EditorTransformComponentSelection::ClearManipulatorTranslationOverride()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_resetManipulatorTranslationUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedTranslation();
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch{ s_resetManipulatorOrientationUndoRedoDesc };

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            m_pivotOverrideFrame.ResetPickedOrientation();
            m_pivotOverrideFrame.m_pickedEntityIdOverride.SetInvalid();

            // parent reference frame is the default (when no modifiers are held)
            m_entityIdManipulators.m_manipulators->SetLocalTransform(AZ::Transform::CreateFromQuaternionAndTranslation(
                ETCS::CalculatePivotOrientationForEntityIds(m_entityIdManipulators.m_lookups, ReferenceFrame::Parent).m_worldOrientation,
                m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation()));

            m_entityIdManipulators.m_manipulators->SetBoundsDirty();

            manipulatorCommand->SetManipulatorAfter(CreateManipulatorCommandStateFromSelf());

            manipulatorCommand->SetParent(undoBatch.GetUndoBatch());
            manipulatorCommand.release();
        }
    }

    void EditorTransformComponentSelection::ToggleCenterPivotSelection()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
        m_pivotMode = TogglePivotMode(m_pivotMode);
        RefreshManipulators(RefreshType::Translation);
    }

    template<typename EntityIdMap>
    static bool ShouldUpdateEntityTransform(const AZ::EntityId entityId, const EntityIdMap& entityIdMap)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoTranslationGroupUndoRedoDesc);

            // store previous translation manipulator position
            const AZ::Vector3 previousPivotTranslation = m_entityIdManipulators.m_manipulators->GetLocalTransform().GetTranslation();

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation),
                m_pivotOverrideFrame.m_pickedEntityIdOverride));

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_mode != Mode::Translation)
        {
            return;
        }

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoTranslationIndividualUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            // refresh the transform pivot override if it's set
            if (m_pivotOverrideFrame.m_translationOverride)
            {
                OverrideManipulatorTranslation(translation);
            }

            manipulatorCommand->SetManipulatorAfter(EntityManipulatorCommand::State(
                BuildPivotOverride(m_pivotOverrideFrame.HasTranslationOverride(), m_pivotOverrideFrame.HasOrientationOverride()),
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    QuaternionFromTransformNoScaling(m_entityIdManipulators.m_manipulators->GetLocalTransform()), translation),
                m_pivotOverrideFrame.m_pickedEntityIdOverride));

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_dittoScaleIndividualWorldUndoRedoDesc);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_dittoScaleIndividualLocalUndoRedoDesc);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch{ s_dittoEntityOrientationIndividualUndoRedoDesc };

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

            ManipulatorEntityIds manipulatorEntityIds;
            BuildSortedEntityIdVectorFromEntityIdMap(m_entityIdManipulators.m_lookups, manipulatorEntityIds.m_entityIds);

            // save initial transforms
            const auto transformsBefore = RecordTransformsBefore(manipulatorEntityIds.m_entityIds);

            // update orientations relative to initial
            for (AZ::EntityId entityId : manipulatorEntityIds.m_entityIds)
            {
                ScopedUndoBatch::MarkEntityDirty(entityId);

                const auto transformIt = transformsBefore.find(entityId);
                if (transformIt != transformsBefore.end())
                {
                    AZ::Transform newWorldFromLocal = transformIt->second;
                    const float scale = newWorldFromLocal.GetUniformScale();
                    newWorldFromLocal.SetRotation(orientation);
                    newWorldFromLocal *= AZ::Transform::CreateUniformScale(scale);

                    SetEntityWorldTransform(entityId, newWorldFromLocal);
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_dittoEntityOrientationGroupUndoRedoDesc);

            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        ScopedUndoBatch undoBatch(s_resetOrientationToParentUndoRedoDesc);
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (m_entityIdManipulators.m_manipulators)
        {
            ScopedUndoBatch undoBatch(s_resetTranslationToParentUndoRedoDesc);

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

    int EditorTransformComponentSelection::GetMenuPosition() const
    {
        return aznumeric_cast<int>(EditorContextMenuOrdering::BOTTOM);
    }

    AZStd::string EditorTransformComponentSelection::GetMenuIdentifier() const
    {
        return "Transform Component";
    }

    void EditorTransformComponentSelection::PopulateEditorGlobalContextMenu(QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
    {
         QAction* action = menu->addAction(QObject::tr(s_togglePivotTitleRightClick));
         QObject::connect(
             action, &QAction::triggered, action,
             [this]()
             {
                 ToggleCenterPivotSelection();
             });
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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

        RegenerateManipulators();
    }

    static void DrawPreviewAxis(
        AzFramework::DebugDisplayRequests& display,
        const AZ::Transform& transform,
        const float axisLength,
        const AzFramework::CameraState& cameraState)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        const int prevState = display.GetState();

        display.DepthWriteOff();
        display.DepthTestOff();
        display.CullOff();

        display.SetLineWidth(4.0f);

        const auto axisFlip = [&transform, &cameraState](const AZ::Vector3& axis) -> float
        {
            return ShouldFlipCameraAxis(
                       AZ::Transform::CreateIdentity(), transform.GetTranslation(), TransformDirectionNoScaling(transform, axis),
                       cameraState)
                ? -1.0f
                : 1.0f;
        };

        display.SetColor(s_fadedXAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafe() * axisLength * axisFlip(AZ::Vector3::CreateAxisX()));
        display.SetColor(s_fadedYAxisColor);
        display.DrawLine(
            transform.GetTranslation(),
            transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafe() * axisLength * axisFlip(AZ::Vector3::CreateAxisY()));
        display.SetColor(s_fadedZAxisColor);
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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        CheckDirtyEntityIds();

        const auto modifiers =
            ViewportInteraction::KeyboardModifiers(ViewportInteraction::TranslateKeyboardModifiers(QApplication::queryKeyboardModifiers()));

        m_cursorState.Update();

        HandleAccents(
            !m_selectedEntityIds.empty(), m_cachedEntityIdUnderCursor, modifiers.Ctrl(), m_hoveredEntityId,
            ViewportInteraction::BuildMouseButtons(QGuiApplication::mouseButtons()), m_boxSelect.Active());

        const ReferenceFrame referenceFrame = m_spaceCluster.m_spaceLock.value_or(ReferenceFrameFromModifiers(modifiers));

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
            if (m_pivotOverrideFrame.m_pickedEntityIdOverride.IsValid())
            {
                const AZ::Transform pickedEntityWorldTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                    ETCS::CalculatePivotOrientation(m_pivotOverrideFrame.m_pickedEntityIdOverride, referenceFrame).m_worldOrientation,
                    CalculatePivotTranslation(m_pivotOverrideFrame.m_pickedEntityIdOverride, m_pivotMode));

                const float scaledSize =
                    s_pivotSize * CalculateScreenToWorldMultiplier(pickedEntityWorldTransform.GetTranslation(), cameraState);

                debugDisplay.DepthWriteOff();
                debugDisplay.DepthTestOff();
                debugDisplay.SetColor(s_pickedOrientationColor);

                debugDisplay.DrawWireSphere(pickedEntityWorldTransform.GetTranslation(), scaledSize);

                debugDisplay.DepthTestOn();
                debugDisplay.DepthWriteOn();
            }

            // check what pivot orientation we are in (based on if a modifier is
            // held to move us from parent to world space or parent to local space)
            // or if we set a pivot override
            const auto pivotResult =
                ETCS::CalculateSelectionPivotOrientation(m_entityIdManipulators.m_lookups, m_pivotOverrideFrame, m_referenceFrame);

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
                        AZ::Vector3(s_pivotSize) * CalculateScreenToWorldMultiplier(worldFromLocal.GetTranslation(), cameraState);

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
        const AZ::Matrix3x3& editorCameraOrientation = AZ::Matrix3x3::CreateFromMatrix4x4(AzFramework::CameraTransform(editorCameraState));

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
        const AZ::Vector2 screenOffset = AZ::Vector2(cl_viewportGizmoAxisScreenPosition) - AZ::Vector2(0.5f, 0.5f);

        // map from a position in world space (relative to the the gizmo camera near the origin) to a position in
        // screen space
        const auto calculateGizmoAxis = [&cameraView, &cameraProjection, &screenOffset](const AZ::Vector3& axis)
        {
            auto result = AZ::Vector2(AzFramework::WorldToScreenNDC(axis, cameraView, cameraProjection));
            result.SetY(1.0f - result.GetY());
            return result + screenOffset;
        };

        // get all important axis positions in screen space
        const float lineLength = cl_viewportGizmoAxisLineLength;
        const auto gizmoStart = calculateGizmoAxis(AZ::Vector3::CreateZero());
        const auto gizmoEndAxisX = calculateGizmoAxis(-AZ::Vector3::CreateAxisX() * lineLength);
        const auto gizmoEndAxisY = calculateGizmoAxis(-AZ::Vector3::CreateAxisY() * lineLength);
        const auto gizmoEndAxisZ = calculateGizmoAxis(-AZ::Vector3::CreateAxisZ() * lineLength);

        const AZ::Vector2 gizmoAxisX = gizmoEndAxisX - gizmoStart;
        const AZ::Vector2 gizmoAxisY = gizmoEndAxisY - gizmoStart;
        const AZ::Vector2 gizmoAxisZ = gizmoEndAxisZ - gizmoStart;

        // draw the axes of the gizmo
        debugDisplay.SetLineWidth(cl_viewportGizmoAxisLineWidth);
        debugDisplay.SetColor(AZ::Colors::Red);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisX, 1.0f);
        debugDisplay.SetColor(AZ::Colors::Lime);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisY, 1.0f);
        debugDisplay.SetColor(AZ::Colors::Blue);
        debugDisplay.DrawLine2d(gizmoStart, gizmoEndAxisZ, 1.0f);
        debugDisplay.SetLineWidth(1.0f);

        const float labelOffset = cl_viewportGizmoAxisLabelOffset;
        const float screenScale = GetScreenDisplayScaling(viewportId);
        const auto labelXScreenPosition = (gizmoStart + (gizmoAxisX * labelOffset)) * editorCameraState.m_viewportSize * screenScale;
        const auto labelYScreenPosition = (gizmoStart + (gizmoAxisY * labelOffset)) * editorCameraState.m_viewportSize * screenScale;
        const auto labelZScreenPosition = (gizmoStart + (gizmoAxisZ * labelOffset)) * editorCameraState.m_viewportSize * screenScale;

        // draw the label of of each axis for the gizmo
        const float labelSize = cl_viewportGizmoAxisLabelSize;
        debugDisplay.SetColor(AZ::Colors::White);
        debugDisplay.Draw2dTextLabel(labelXScreenPosition.GetX(), labelXScreenPosition.GetY(), labelSize, "X", true);
        debugDisplay.Draw2dTextLabel(labelYScreenPosition.GetX(), labelYScreenPosition.GetY(), labelSize, "Y", true);
        debugDisplay.Draw2dTextLabel(labelZScreenPosition.GetX(), labelZScreenPosition.GetY(), labelSize, "Z", true);
    }

    void EditorTransformComponentSelection::DisplayViewportSelection2d(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        DrawAxisGizmo(viewportInfo, debugDisplay);

        m_boxSelect.Display2d(viewportInfo, debugDisplay);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // check what the 'authoritative' selected entity ids are after an undo/redo
        EntityIdList selectedEntityIds;
        ToolsApplicationRequests::Bus::BroadcastResult(selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

        RefreshSelectedEntityIds(selectedEntityIds);
    }

    void EditorTransformComponentSelection::RefreshSelectedEntityIds(const EntityIdList& selectedEntityIds)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

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
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_transformChangedInternally)
        {
            RefreshManipulators(RefreshType::All);
        }
    }

    void EditorTransformComponentSelection::OnViewportViewEntityChanged(const AZ::EntityId& newViewId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        // if a viewport view entity has been set (e.g. we have set EditorCameraComponent to
        // match the editor camera translation/orientation), record the entity id if we have
        // a manipulator tracking it (entity id exists in m_entityIdManipulator lookups)
        // and remove it when recreating manipulators (see InitializeManipulators)
        if (newViewId.IsValid())
        {
            const auto entityIdLookupIt = m_entityIdManipulators.m_lookups.find(newViewId);
            if (entityIdLookupIt != m_entityIdManipulators.m_lookups.end())
            {
                m_editorCameraComponentEntityId = newViewId;
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

    void EditorTransformComponentSelection::EnteredComponentMode([[maybe_unused]] const AZStd::vector<AZ::Uuid>& componentModeTypes)
    {
        SetAllViewportUiVisible(false);

        EditorEntityLockComponentNotificationBus::Router::BusRouterDisconnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterDisconnect();
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
    }

    void EditorTransformComponentSelection::LeftComponentMode([[maybe_unused]] const AZStd::vector<AZ::Uuid>& componentModeTypes)
    {
        SetAllViewportUiVisible(true);

        ToolsApplicationNotificationBus::Handler::BusConnect();
        EditorEntityVisibilityNotificationBus::Router::BusRouterConnect();
        EditorEntityLockComponentNotificationBus::Router::BusRouterConnect();
    }

    void EditorTransformComponentSelection::CreateEntityManipulatorDeselectCommand(ScopedUndoBatch& undoBatch)
    {
        if (m_entityIdManipulators.m_manipulators)
        {
            auto manipulatorCommand =
                AZStd::make_unique<EntityManipulatorCommand>(CreateManipulatorCommandStateFromSelf(), s_manipulatorUndoRedoName);

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
        ETCS::SetEntityWorldTranslation(entityId, worldTranslation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalTranslation(const AZ::EntityId entityId, const AZ::Vector3& localTranslation)
    {
        ETCS::SetEntityLocalTranslation(entityId, localTranslation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityWorldTransform(const AZ::EntityId entityId, const AZ::Transform& worldTransform)
    {
        ETCS::SetEntityWorldTransform(entityId, worldTransform, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalScale(const AZ::EntityId entityId, const float localScale)
    {
        ETCS::SetEntityLocalScale(entityId, localScale, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::SetEntityLocalRotation(const AZ::EntityId entityId, const AZ::Vector3& localRotation)
    {
        ETCS::SetEntityLocalRotation(entityId, localRotation, m_transformChangedInternally);
    }

    void EditorTransformComponentSelection::OnStartPlayInEditor()
    {
        SetAllViewportUiVisible(false);
    }

    void EditorTransformComponentSelection::OnStopPlayInEditor()
    {
        SetAllViewportUiVisible(true);
    }

    namespace ETCS
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
    } // namespace ETCS

    // explicit instantiations
    template ETCS::PivotOrientationResult ETCS::CalculatePivotOrientationForEntityIds<EntityIdManipulatorLookups>(
        const EntityIdManipulatorLookups&, ReferenceFrame);
    template ETCS::PivotOrientationResult ETCS::CalculateSelectionPivotOrientation<EntityIdManipulatorLookups>(
        const EntityIdManipulatorLookups&, const OptionalFrame&, const ReferenceFrame referenceFrame);
} // namespace AzToolsFramework
