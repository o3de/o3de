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

#include "EditorCommon.h"
#include "CommandCanvasSize.h"

CommandCanvasSize::CommandCanvasSize(UndoStack* stack,
    CanvasSizeToolbarSection* canvasSizeToolbar,
    AZ::Vector2 from,
    AZ::Vector2 to,
    bool fromPreset)
    : QUndoCommand()
    , m_stack(stack)
    , m_canvasSizeToolbar(canvasSizeToolbar)
    , m_from(from)
    , m_to(to)
    , m_fromPreset(fromPreset)
{
    UpdateText();
}

void CommandCanvasSize::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetSize(m_from, m_fromPreset);
}

void CommandCanvasSize::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetSize(m_to, false);
}

void CommandCanvasSize::UpdateText()
{
    QString description(
        QString("%1 x %2 (%3)")
        .arg(QString::number(m_to.GetX()))
        .arg(QString::number(m_to.GetY()))
        .arg("custom"));

    setText(QString("canvas size change to %1").arg(description));
}

void CommandCanvasSize::SetSize(AZ::Vector2 size, bool fromPreset) const
{
    // IMPORTANT: It's NOT necessary to prevent this from executing on the
    // first run. We WON'T get a redundant Qt notification by this point.
    m_canvasSizeToolbar->SetCustomCanvasSize(size, fromPreset);
}

void CommandCanvasSize::Push(UndoStack* stack,
    CanvasSizeToolbarSection* canvasSizeToolbar,
    AZ::Vector2 from,
    AZ::Vector2 to,
    bool fromPreset)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandCanvasSize(stack, canvasSizeToolbar, from, to, fromPreset));
}
