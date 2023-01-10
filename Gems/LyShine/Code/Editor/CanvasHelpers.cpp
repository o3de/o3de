/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>

namespace CanvasHelpers
{
    AZStd::string BeginUndoableCanvasChange(AZ::EntityId canvasEntityId)
    {
        // Currently this serializes the entire canvas including all elements.
        AZStd::string canvasUndoXml;
        UiCanvasBus::EventResult(canvasUndoXml, canvasEntityId, &UiCanvasBus::Events::SaveToXmlString);
        return canvasUndoXml;
    }

    void EndUndoableCanvasChange(EditorWindow* editorWindow, const char* commandName, AZStd::string& canvasUndoXml)
    {
        // serialize the changed state of the canvas
        AZStd::string canvasRedoXml;
        UiCanvasBus::EventResult(canvasRedoXml, editorWindow->GetCanvas(), &UiCanvasBus::Events::SaveToXmlString);
        if (canvasUndoXml.empty() || canvasRedoXml.empty())
        {
            AZ_Warning("UI", false, "Failed to serialize canvas for undo of %s'.", commandName);
        }
        else
        {
            // Create the undoable command and push it onto the undo stack
            CommandCanvasPropertiesChange::Push(editorWindow->GetActiveStack(), canvasUndoXml, canvasRedoXml, editorWindow, commandName);
        }
    }

    AZ::Vector2 GetViewportPoint(AZ::EntityId canvasEntityId, const AZ::Vector2& canvasPoint)
    {
        AZ::Matrix4x4 transform;
        UiCanvasBus::EventResult(transform, canvasEntityId, &UiCanvasBus::Events::GetCanvasToViewportMatrix);

        AZ::Vector3 canvasPoint3(canvasPoint.GetX(), canvasPoint.GetY(), 0.0f);
        AZ::Vector3 viewportPoint3 = transform * canvasPoint3;
        AZ::Vector2 viewportPoint(viewportPoint3.GetX(), viewportPoint3.GetY());

        return viewportPoint;
    }

    AZ::Vector2 GetSnappedCanvasPoint(AZ::EntityId canvasEntityId, const AZ::Vector2& viewportPoint, bool snapToGrid)
    {
        AZ::Matrix4x4 transform;
        UiCanvasBus::Event(canvasEntityId, &UiCanvasBus::Events::GetViewportToCanvasMatrix, transform);

        AZ::Vector3 viewportPoint3(viewportPoint.GetX(), viewportPoint.GetY(), 0.0f);
        AZ::Vector3 canvasPoint3 = transform * viewportPoint3;
        AZ::Vector2 canvasPoint(canvasPoint3.GetX(), canvasPoint3.GetY());

        // if the snapping flag is off we still always snap to the nearest pixel
        float snapDistance = 1.0f;
        if (snapToGrid)
        {
            UiEditorCanvasBus::EventResult(snapDistance, canvasEntityId, &UiEditorCanvasBus::Events::GetSnapDistance);
        }

        return EntityHelpers::Snap(canvasPoint, snapDistance);
    }


}   // namespace CanvasHelpers
