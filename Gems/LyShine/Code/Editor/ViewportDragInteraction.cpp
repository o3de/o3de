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
#include "UiCanvasEditor_precompiled.h"

#include "ViewportDragInteraction.h"

ViewportDragInteraction::ViewportDragInteraction(const AZ::Vector2& startMousePos)
    : m_startMousePos(startMousePos)
{
}

ViewportDragInteraction::~ViewportDragInteraction()
{
}

void ViewportDragInteraction::Render([[maybe_unused]] Draw2dHelper& draw2d)
{
}

void ViewportDragInteraction::EndInteraction(EndState /*endState*/)
{
}

