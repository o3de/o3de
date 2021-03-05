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
