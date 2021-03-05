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

namespace GuideHelpers
{
    //! Find the guide close to the given point
    bool PickGuide(AZ::EntityId canvasEntityId, const AZ::Vector2& point, bool& guideIsVertical, int& guideIndex);

    //! Get the position of a given guide
    float GetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex);

    //! Set the position of a given guide (given a float)
    void SetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex, float pos);

    //! Set the position of a given guide (given a point)
    void SetGuidePosition(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex, const AZ::Vector2& pos);

    //! Remove a guide
    void RemoveGuide(AZ::EntityId canvasEntityId, bool guideIsVertical, int guideIndex);

    //! Set whether guides are locked for this canvas
    void SetGuidesAreLocked(AZ::EntityId canvasEntityId, bool areLocked);

    //! Get whether guides are locked for this canvas
    bool AreGuidesLocked(AZ::EntityId canvasEntityId);

    //! Draw the guide lines on the canvas
    void DrawGuideLines(AZ::EntityId canvasEntityId, ViewportWidget* viewport, Draw2dHelper& draw2d);

    //! Draw the ghost guide line used when adding a guide
    void DrawGhostGuideLine(Draw2dHelper& draw2d, AZ::EntityId canvasEntityId, bool guideIsVertical, ViewportWidget* viewport, const AZ::Vector2& canvasPoint);

    //! Draw the guide position next to the cursor position
    void DrawGuidePosTextDisplay(Draw2dHelper& draw2d, bool guideIsVertical, float guidePos, const ViewportWidget* viewport);

}   // namespace GuideHelpers
