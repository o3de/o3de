/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "ViewportAddGuideInteraction.h"
#include "CanvasHelpers.h"
#include "GuideHelpers.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>


ViewportAddGuideInteraction::ViewportAddGuideInteraction(
    EditorWindow* editorWindow,
    AZ::EntityId canvasId,
    bool guideIsVertical,
    const AZ::Vector2& startDragMousePos
)
    : ViewportDragInteraction(startDragMousePos)
    , m_editorWindow(editorWindow)
    , m_canvasId(canvasId)
    , m_guideIsVertical(guideIsVertical)
    , m_cursorViewportPos(0.0f, 0.0f)
{
    // store whether snapping is enabled for this canvas
    UiEditorCanvasBus::EventResult(m_isSnapping, canvasId, &UiEditorCanvasBus::Events::GetIsSnapEnabled);

    m_addingGuideAtPosition = CanvasHelpers::GetSnappedCanvasPoint(m_canvasId, startDragMousePos, m_isSnapping);
}

ViewportAddGuideInteraction::~ViewportAddGuideInteraction()
{
}

void ViewportAddGuideInteraction::Update(const AZ::Vector2& mousePos)
{
    m_cursorViewportPos = mousePos;
    m_addingGuideAtPosition = CanvasHelpers::GetSnappedCanvasPoint(m_canvasId, mousePos, m_isSnapping);

}

void ViewportAddGuideInteraction::Render(Draw2dHelper& draw2d)
{
    float pos = (m_guideIsVertical) ? m_addingGuideAtPosition.GetX() : m_addingGuideAtPosition.GetY(); 

    GuideHelpers::DrawGhostGuideLine(draw2d, m_editorWindow->GetCanvas(), m_guideIsVertical, m_editorWindow->GetViewport(), m_addingGuideAtPosition);
    GuideHelpers::DrawGuidePosTextDisplay(draw2d, m_guideIsVertical, pos, m_editorWindow->GetViewport());
}

void ViewportAddGuideInteraction::EndInteraction(EndState endState)
{
    if (endState == EndState::Inside || (m_guideIsVertical ? endState == EndState::OutsideY : endState == EndState::OutsideX))
    {
        // The drag was released in the viewport
        AZ::EntityId canvasEntityId = m_editorWindow->GetCanvas();

        // record canvas state before the change
        AZStd::string canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(canvasEntityId);

        // Add the new guide to the canvas
        if (m_guideIsVertical)
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::AddVerticalGuide, m_addingGuideAtPosition.GetX());
        }
        else
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::AddHorizontalGuide, m_addingGuideAtPosition.GetY());
        }

        // force guides to be visible so that you can see the added guide
        m_editorWindow->GetViewport()->ShowGuides(true);
        
        // Create the undoable command and push it onto the undo stack
        CanvasHelpers::EndUndoableCanvasChange(m_editorWindow, "add guide", canvasUndoXml); 
    }
}
