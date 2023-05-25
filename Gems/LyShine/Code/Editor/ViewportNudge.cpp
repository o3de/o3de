/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "ViewportNudge.h"

static const float slowNudgePixelDistance = 1.0f;
static const float fastNudgePixelDistance = 10.0f;
static const float slowNudgeRotationDegrees = 1.0f;
static const float fastNudgeRotationDegrees = 10.0f;

void ViewportNudge::Nudge(
    EditorWindow* editorWindow,
    ViewportInteraction::InteractionMode interactionMode,
    [[maybe_unused]] ViewportWidget* viewport,
    ViewportInteraction::NudgeDirection direction,
    ViewportInteraction::NudgeSpeed speed,
    const QTreeWidgetItemRawPtrQList& selectedItems,
    ViewportInteraction::CoordinateSystem coordinateSystem,
    const AZ::Uuid& transformComponentType)
{
    bool isMoveOrAnchorMode = interactionMode == ViewportInteraction::InteractionMode::MOVE || interactionMode == ViewportInteraction::InteractionMode::ANCHOR;

    if (isMoveOrAnchorMode)
    {
        float translation = slowNudgePixelDistance;
        switch (speed)
        {
        case ViewportInteraction::NudgeSpeed::Fast:
        {
            translation = fastNudgePixelDistance;
        }
        break;

        case ViewportInteraction::NudgeSpeed::Slow:
        {
            translation = slowNudgePixelDistance;
        }
        break;

        default:
        {
            AZ_Assert(0, "Unknown speed");
        }
        break;
        }

        // Translate.
        {
            auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElementsNotControlledByParent(editorWindow->GetHierarchy(), selectedItems);
            if (topLevelSelectedElements.empty())
            {
                // Nothing to do.
                return;
            }

            SerializeHelpers::SerializedEntryList preChangeState;
            HierarchyClipboard::BeginUndoableEntitiesChange(editorWindow, preChangeState);

            for (auto element : topLevelSelectedElements)
            {
                AZ::Vector2 deltaInCanvasSpace(0.0f, 0.0f);
                if (direction == ViewportInteraction::NudgeDirection::Up)
                {
                    deltaInCanvasSpace.SetY(-translation);
                }
                else if (direction == ViewportInteraction::NudgeDirection::Down)
                {
                    deltaInCanvasSpace.SetY(translation);
                }
                else if (direction == ViewportInteraction::NudgeDirection::Left)
                {
                    deltaInCanvasSpace.SetX(-translation);
                }
                else if (direction == ViewportInteraction::NudgeDirection::Right)
                {
                    deltaInCanvasSpace.SetX(translation);
                }
                else
                {
                    AZ_Assert(0, "Unexpected direction.");
                }

                AZ::EntityId parentEntityId = EntityHelpers::GetParentElement(element->GetId())->GetId();

                AZ::Vector2 deltaInLocalSpace;
                if (coordinateSystem == ViewportInteraction::CoordinateSystem::LOCAL)
                {
                    deltaInLocalSpace = deltaInCanvasSpace;
                }
                else // if (coordinateSystem == ViewportInteraction::CoordinateSystem::VIEW)
                {
                    // compute the delta to move in local space (i.e. relative to the parent)
                    deltaInLocalSpace = EntityHelpers::TransformDeltaFromCanvasToLocalSpace(parentEntityId, deltaInCanvasSpace);
                }

                if (interactionMode == ViewportInteraction::InteractionMode::MOVE)
                {
                    EntityHelpers::MoveByLocalDeltaUsingOffsets(element->GetId(), deltaInLocalSpace);
                }
                else if (interactionMode == ViewportInteraction::InteractionMode::ANCHOR)
                {
                    EntityHelpers::MoveByLocalDeltaUsingAnchors(element->GetId(), parentEntityId, deltaInLocalSpace, true);
                }

                // Update.
                UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
            }

            // Tell the Properties panel to update
            editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

            HierarchyClipboard::EndUndoableEntitiesChange(editorWindow, "nudge move", preChangeState);
        }
    }
    else if (interactionMode == ViewportInteraction::InteractionMode::ROTATE)
    {
        float rotationDirection = 1.0f;

        float rotationDeltaInDegrees = slowNudgeRotationDegrees;
        switch (speed)
        {
        case ViewportInteraction::NudgeSpeed::Fast:
        {
            rotationDeltaInDegrees = fastNudgeRotationDegrees;
        }
        break;

        case ViewportInteraction::NudgeSpeed::Slow:
        {
            rotationDeltaInDegrees = slowNudgeRotationDegrees;
        }
        break;

        default:
        {
            AZ_Assert(0, "Unknown speed");
        }
        break;
        }

        {
            if (direction == ViewportInteraction::NudgeDirection::Up ||
                (direction == ViewportInteraction::NudgeDirection::Left))
            {
                rotationDirection = -1.0f;
            }
            else if ((direction == ViewportInteraction::NudgeDirection::Down) ||
                     (direction == ViewportInteraction::NudgeDirection::Right))
            {
                rotationDirection = 1.0f;
            }
            else
            {
                AZ_Assert(0, "Unexpected direction.");
                return;
            }
        }

        // Rotate.
        {
            auto topLevelSelectedElements = SelectionHelpers::GetTopLevelSelectedElements(editorWindow->GetHierarchy(), selectedItems);
            if (topLevelSelectedElements.empty())
            {
                // Nothing to do.
                return;
            }

            SerializeHelpers::SerializedEntryList preChangeState;
            HierarchyClipboard::BeginUndoableEntitiesChange(editorWindow, preChangeState);

            for (auto element : topLevelSelectedElements)
            {
                // Get.
                float rotation;
                UiTransformBus::EventResult(rotation, element->GetId(), &UiTransformBus::Events::GetZRotation);

                // Set.
                UiTransformBus::Event(
                    element->GetId(), &UiTransformBus::Events::SetZRotation, (rotation + (rotationDeltaInDegrees * rotationDirection)));

                // Update.
                UiElementChangeNotificationBus::Event(element->GetId(), &UiElementChangeNotificationBus::Events::UiElementPropertyChanged);
            }

            // Tell the Properties panel to update
            editorWindow->GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values, &transformComponentType);

            HierarchyClipboard::EndUndoableEntitiesChange(editorWindow, "nudge rotate", preChangeState);
        }
    }
}
