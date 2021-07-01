/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

CommandCanvasSizeToolbarIndex::CommandCanvasSizeToolbarIndex(UndoStack* stack,
    CanvasSizeToolbarSection* canvasSizeToolbar,
    int from,
    int to)
    : QUndoCommand()
    , m_stack(stack)
    , m_canvasSizeToolbar(canvasSizeToolbar)
    , m_from(from)
    , m_to(to)
{
    UpdateText();
}

void CommandCanvasSizeToolbarIndex::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetIndex(m_from);
}

void CommandCanvasSizeToolbarIndex::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetIndex(m_to);
}

int CommandCanvasSizeToolbarIndex::id() const
{
    return (int)FusibleCommand::kCanvasSizeToolbarIndex;
}

bool CommandCanvasSizeToolbarIndex::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id())
    {
        // NOT the same command type.
        return false;
    }

    const CommandCanvasSizeToolbarIndex* subsequent = static_cast<const CommandCanvasSizeToolbarIndex*>(other);
    AZ_Assert(subsequent, "There is no command to merge with");

    if (!((subsequent->m_stack == m_stack) &&
          (subsequent->m_canvasSizeToolbar == m_canvasSizeToolbar)))
    {
        // NOT the same context.
        return false;
    }

    m_to = subsequent->m_to;

    UpdateText();

    return true;
}

void CommandCanvasSizeToolbarIndex::UpdateText()
{
    setText(QString("canvas size change to %1").arg(m_canvasSizeToolbar->IndexToString(m_to)));
}

void CommandCanvasSizeToolbarIndex::SetIndex(int index) const
{
    // IMPORTANT: It's NOT necessary to prevent this from executing on the
    // first run. We WON'T get a redundant Qt notification by this point.
    m_canvasSizeToolbar->SetIndex(index);
}

void CommandCanvasSizeToolbarIndex::Push(UndoStack* stack,
    CanvasSizeToolbarSection* canvasSizeToolbar,
    int from,
    int to)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandCanvasSizeToolbarIndex(stack, canvasSizeToolbar, from, to));
}
