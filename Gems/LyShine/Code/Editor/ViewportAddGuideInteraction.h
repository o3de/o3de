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

#include "ViewportDragInteraction.h"
#include "ViewportInteraction.h"

#include <LyShine/Bus/UiTransform2dBus.h>

class EditorWindow;

//! Class used while an "Add guide" interaction is in progress in move or anchor mode
class ViewportAddGuideInteraction
    : public ViewportDragInteraction
{
public:

    ViewportAddGuideInteraction(
        EditorWindow* editorWindow,
        AZ::EntityId canvasId,
        bool guideIsVertical,
        const AZ::Vector2& startDragMousePos);

    virtual ~ViewportAddGuideInteraction();

    // ViewportDragInteraction
    void Update(const AZ::Vector2& mousePos) override;
    void Render(Draw2dHelper& draw2d) override;
    void EndInteraction(EndState endState) override;
    // ~ViewportDragInteraction

protected:

    // State that we will need every frame in the update is cached locally in this object
    EditorWindow* m_editorWindow;
    AZ::EntityId m_canvasId;
    bool m_guideIsVertical = false;
    bool m_isSnapping = false;

    // State that changes during the interaction
    AZ::Vector2 m_addingGuideAtPosition;
    AZ::Vector2 m_cursorViewportPos;
};
