/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
