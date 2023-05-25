/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "ViewportMoveGuideInteraction.h"
#include "CanvasHelpers.h"
#include "GuideHelpers.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>


ViewportMoveGuideInteraction::ViewportMoveGuideInteraction(
    EditorWindow* editorWindow,
    AZ::EntityId canvasId,
    bool guideIsVertical,
    int guideIndex,
    const AZ::Vector2& startDragMousePos
)
    : ViewportDragInteraction(startDragMousePos)
    , m_editorWindow(editorWindow)
    , m_canvasId(canvasId)
    , m_guideIsVertical(guideIsVertical)
    , m_guideIndex(guideIndex)
    , m_cursorViewportPos(0.0f, 0.0f)
{
    // store whether snapping is enabled for this canvas
    UiEditorCanvasBus::EventResult(m_isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

    m_startingPosition = GuideHelpers::GetGuidePosition(canvasId, m_guideIsVertical, m_guideIndex);

    // store the state before anything is moved
    m_canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(m_canvasId);
}

ViewportMoveGuideInteraction::~ViewportMoveGuideInteraction()
{
}

void ViewportMoveGuideInteraction::Update(const AZ::Vector2& mousePos)
{
    // remember mouse position for render
    m_cursorViewportPos = mousePos;

    // Move the guide
    MoveGuideToMousePos(mousePos);
}

void ViewportMoveGuideInteraction::Render(Draw2dHelper& draw2d)
{
    // We don't need to render the guide since its position has been updated and the normal canvas render will draw it
    // What we draw is the "visual aid" which in this case is a text display of the ruler position
    float pos = GuideHelpers::GetGuidePosition(m_canvasId, m_guideIsVertical, m_guideIndex);
    GuideHelpers::DrawGuidePosTextDisplay(draw2d, m_guideIsVertical, pos, m_editorWindow->GetViewport());
}

void ViewportMoveGuideInteraction::EndInteraction(EndState endState)
{
    // If the interaction ended inside the viewport then do a move, else do a delete
    const char* commandName = "move guide";
    if (endState == EndState::OutsideXY || (m_guideIsVertical ? endState == EndState::OutsideX : endState == EndState::OutsideY))
    {
        GuideHelpers::RemoveGuide(m_canvasId, m_guideIsVertical, m_guideIndex);
        commandName = "delete guide";
    }

    CanvasHelpers::EndUndoableCanvasChange(m_editorWindow, commandName, m_canvasUndoXml);
}

void ViewportMoveGuideInteraction::MoveGuideToMousePos(const AZ::Vector2& viewportPos)
{
    AZ::Vector2 snappedPos = CanvasHelpers::GetSnappedCanvasPoint(m_canvasId, viewportPos, m_isSnapping);

    GuideHelpers::SetGuidePosition(m_canvasId, m_guideIsVertical, m_guideIndex, snappedPos);
}
