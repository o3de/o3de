/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QUndoCommand>

class CommandViewportInteractionMode
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    static void Push(UndoStack* stack,
        ViewportInteraction* viewportInteraction,
        QAction* from,
        QAction* to);

private:

    CommandViewportInteractionMode(UndoStack* stack,
        ViewportInteraction* viewportInteraction,
        QAction* from,
        QAction* to);

    void UpdateText();
    void SetMode(QAction* action) const;

    UndoStack* m_stack;

    ViewportInteraction* m_viewportInteraction;
    QAction* m_from;
    QAction* m_to;
};
