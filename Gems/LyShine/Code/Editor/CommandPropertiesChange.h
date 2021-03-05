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

class CommandPropertiesChange
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        SerializeHelpers::SerializedEntryList& preValueChanges,
        const char* commandName);

private:

    CommandPropertiesChange(UndoStack* stack,
        HierarchyWidget* hierarchy,
        SerializeHelpers::SerializedEntryList& preValueChanges,
        const char* commandName);

    void Recreate(bool isUndo);

    UndoStack* m_stack;

    // The first execution of redo() is done in REACTION to a Qt
    // event that has ALREADY completed the necessary work. We ONLY
    // want to execute redo() on SUBSEQUENT calls.
    bool m_isFirstExecution;

    // This command can fail because of missing parents.
    // When it does, we don't want to try to execute it again.
    bool m_hasPreviouslyFailed;

    HierarchyWidget* m_hierarchy;

    SerializeHelpers::SerializedEntryList m_entryList;
};
