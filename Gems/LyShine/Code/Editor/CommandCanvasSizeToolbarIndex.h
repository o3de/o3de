/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QUndoCommand>

class CommandCanvasSizeToolbarIndex
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    int id() const override;
    bool mergeWith(const QUndoCommand* other) override;

    static void Push(UndoStack* stack,
        CanvasSizeToolbarSection* canvasSizeToolbar,
        int from,
        int to);

private:

    CommandCanvasSizeToolbarIndex(UndoStack* stack,
        CanvasSizeToolbarSection* canvasSizeToolbar,
        int from,
        int to);

    void UpdateText();
    void SetIndex(int index) const;

    UndoStack* m_stack;

    CanvasSizeToolbarSection* m_canvasSizeToolbar;
    int m_from;
    int m_to;
};
