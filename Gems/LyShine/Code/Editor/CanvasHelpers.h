/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace CanvasHelpers
{
    //! Begin an undoable canvas change
    AZStd::string BeginUndoableCanvasChange(AZ::EntityId canvasEntityId);

    //! End an undoable canvas change
    void EndUndoableCanvasChange(EditorWindow* editorWindow, const char* commandName, AZStd::string& canvasUndoXml);

    //! Given a point in canvas space return the point in viewport space
    AZ::Vector2 GetViewportPoint(AZ::EntityId canvasEntityId, const AZ::Vector2& canvasPoint);

    //! Given a point in viewport space convert to canvas space and snap. If snapToGrid is true it snaps to canvas snap distance, else to nearest pixel
    AZ::Vector2 GetSnappedCanvasPoint(AZ::EntityId canvasEntityId, const AZ::Vector2& viewportPoint, bool snapToGrid);

}   // namespace CanvasHelpers
