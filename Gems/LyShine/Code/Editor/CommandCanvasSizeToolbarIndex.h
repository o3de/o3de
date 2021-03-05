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
