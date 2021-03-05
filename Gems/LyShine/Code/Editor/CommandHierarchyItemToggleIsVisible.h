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

class CommandHierarchyItemToggleIsVisible
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    // IMPORTANT: We DON'T want this command to support mergeWith().
    // Otherwise we leave commands on the undo stack that have no
    // effect (NOOP).
    //
    // To avoid the NOOPs, we can either:
    //
    // (1) Delete the NOPs from the undo stack.
    // or
    // (2) NOT support mergeWith().
    //
    // The problem with (1) is that it only allows odd number of
    // state changes to be undoable. (2) is more consistent
    // by making all state changes undoable.

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const HierarchyItemRawPtrList& items);

private:

    CommandHierarchyItemToggleIsVisible(UndoStack* stack,
        HierarchyWidget* hierarchy,
        const HierarchyItemRawPtrList& items);

    void SetIsVisible(bool isVisible);

    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;

    EntityHelpers::EntityIdList m_ids;
    bool m_toIsVisible;
};
