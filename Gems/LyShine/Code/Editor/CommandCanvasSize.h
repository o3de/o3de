/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QUndoCommand>

class CommandCanvasSize
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        CanvasSizeToolbarSection* canvasSizeToolbar,
        AZ::Vector2 from,
        AZ::Vector2 to,
        bool fromPreset);

private:

    CommandCanvasSize(UndoStack* stack,
        CanvasSizeToolbarSection* canvasSizeToolbar,
        AZ::Vector2 from,
        AZ::Vector2 to,
        bool fromPreset);

    void UpdateText();
    void SetSize(AZ::Vector2 size, bool fromPreset) const;

    UndoStack* m_stack;

    CanvasSizeToolbarSection* m_canvasSizeToolbar;
    AZ::Vector2 m_from;
    AZ::Vector2 m_to;
    bool m_fromPreset;
};
