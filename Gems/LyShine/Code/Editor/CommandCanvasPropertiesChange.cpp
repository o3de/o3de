/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

CommandCanvasPropertiesChange::CommandCanvasPropertiesChange(UndoStack* stack,
    AZStd::string& undoXml, AZStd::string& redoXml, EditorWindow* editorWindow, const char* commandName)
    : QUndoCommand()
    , m_stack(stack)
    , m_isFirstExecution(true)
    , m_selectionWasEmpty(true)
    , m_undoXml(undoXml)
    , m_redoXml(redoXml)
    , m_editorWindow(editorWindow)
{
    setText(commandName);
}

void CommandCanvasPropertiesChange::undo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(true);

    // Some canvas properties (such as whether guides are locked) affect the menus
    m_editorWindow->RefreshEditorMenu();
}

void CommandCanvasPropertiesChange::redo()
{
    UndoStackExecutionScope s(m_stack);

    Recreate(false);

    // Some canvas properties (such as whether guides are locked) affect the menus
    m_editorWindow->RefreshEditorMenu();
}

void CommandCanvasPropertiesChange::Recreate(bool isUndo)
{
    if (m_isFirstExecution)
    {
        m_isFirstExecution = false;

        HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
        if (hierarchyWidget)
        {
            m_selectionWasEmpty = (hierarchyWidget->CurrentSelectedElement()) ? false : true;
        }

        // Nothing else to do.
    }
    else
    {
        // We are going to load a saved canvas from XML and replace the existing canvas
        // with it. 
        // Create a new entity context for the new canvas
        UiEditorEntityContext* newEntityContext = new UiEditorEntityContext(m_editorWindow);

        // Create a new canvas from the XML and release the old canvas, use the new entity context for
        // the new canvas
        const AZStd::string& xml = isUndo ? m_undoXml : m_redoXml;
        AZ::Interface<ILyShine>::Get()->ReloadCanvasFromXml(xml, newEntityContext);

        // Tell the editor window to use the new entity context
        m_editorWindow->ReplaceEntityContext(newEntityContext);

        // Tell the UI animation system that the active canvas has changed
        UiEditorAnimationBus::Broadcast(&UiEditorAnimationBus::Events::ActiveCanvasChanged);

        // Some toolbar sections display canvas properties
        ViewportWidget* viewportWidget = m_editorWindow->GetViewport();
        if (viewportWidget)
        {
            viewportWidget->GetViewportInteraction()->InitializeToolbars();
        }

        // Clear any selected elements from the hierarchy widget. If an element is selected,
        // this will trigger the properties pane to refresh with the new canvas, but the
        // refresh is on a timer so it won't happen right away.
        HierarchyWidget* hierarchyWidget = m_editorWindow->GetHierarchy();
        if (hierarchyWidget)
        {
            // check if the selection was empty when the command was executed,
            // if so we set the selection back to empty so that the properties pane shows the
            // canvas properties and the result of the undo can be seen
            if (m_selectionWasEmpty)
            {
                hierarchyWidget->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);
            }
        }

        // Tell the properties pane that the entity pointer changed
        PropertiesWidget* propertiesWidget = m_editorWindow->GetProperties();
        if (propertiesWidget)
        {
            propertiesWidget->SelectedEntityPointersChanged();
        }
    }
}

void CommandCanvasPropertiesChange::Push(UndoStack* stack, AZStd::string& undoXml,
    AZStd::string& redoXml, EditorWindow* editorWindow, const char* commandName)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandCanvasPropertiesChange(stack, undoXml, redoXml, editorWindow, commandName));
}
