/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandViewportInteractionMode::CommandViewportInteractionMode(UndoStack* stack,
    ViewportInteraction* viewportInteraction,
    QAction* from,
    QAction* to)
    : QUndoCommand()
    , m_stack(stack)
    , m_viewportInteraction(viewportInteraction)
    , m_from(from)
    , m_to(to)
{
    UpdateText();
}

void CommandViewportInteractionMode::undo()
{
    UndoStackExecutionScope s(m_stack);

    SetMode(m_from);
}

void CommandViewportInteractionMode::redo()
{
    UndoStackExecutionScope s(m_stack);

    SetMode(m_to);
}

int CommandViewportInteractionMode::id() const
{
    return (int)FusibleCommand::kViewportInteractionMode;
}

bool CommandViewportInteractionMode::mergeWith(const QUndoCommand* other)
{
    if (other->id() != id())
    {
        // NOT the same command type.
        return false;
    }

    const CommandViewportInteractionMode* subsequent = static_cast<const CommandViewportInteractionMode*>(other);
    AZ_Assert(subsequent, "No command to merge with");

    if (!((subsequent->m_stack == m_stack) &&
          (subsequent->m_viewportInteraction == m_viewportInteraction)))
    {
        // NOT the same context.
        return false;
    }

    m_to = subsequent->m_to;

    UpdateText();

    return true;
}

void CommandViewportInteractionMode::UpdateText()
{
    setText(QString("mode change to %1").arg(ViewportHelpers::InteractionModeToString(m_to->data().toInt())));
}

void CommandViewportInteractionMode::SetMode(QAction* action) const
{
    action->trigger();

    // IMPORTANT: It's NOT necessary to prevent this from executing on the
    // first run. We WON'T get a redundant Qt notification by this point.
    m_viewportInteraction->SetMode((ViewportInteraction::InteractionMode)action->data().toInt());
}

void CommandViewportInteractionMode::Push(UndoStack* stack,
    ViewportInteraction* viewportInteraction,
    QAction* from,
    QAction* to)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandViewportInteractionMode(stack, viewportInteraction, from, to));
}
