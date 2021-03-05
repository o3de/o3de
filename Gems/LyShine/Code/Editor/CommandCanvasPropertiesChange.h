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

class CommandCanvasPropertiesChange
    : public QUndoCommand
{
public:

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        AZStd::string& undoXml,
        AZStd::string& redoXml,
        EditorWindow* editorWindow,
        const char* commandName);

private:

    CommandCanvasPropertiesChange(UndoStack* stack,
        AZStd::string& undoXml,
        AZStd::string& redoXml,
        EditorWindow* editorWindow,
        const char* commandName);

    void Recreate(bool isUndo);

    UndoStack* m_stack;

    // The first execution of redo() is done in REACTION to a Qt
    // event that has ALREADY completed the necessary work. We ONLY
    // want to execute redo() on SUBSEQUENT calls.
    bool m_isFirstExecution;

    // If the selection was empty when the command first occured then it should be set to empty on undo/redo
    // This is so that the use can see the change in the properties pane on undo/redo
    bool m_selectionWasEmpty;

    AZStd::string m_undoXml;
    AZStd::string m_redoXml;

    EditorWindow* m_editorWindow;
};
