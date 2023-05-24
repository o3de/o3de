/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#define UICANVASEDITOR_ELEMENT_NAME_DEFAULT     "Element"

CommandHierarchyItemCreate::CommandHierarchyItemCreate(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const EntityHelpers::EntityIdList& parents,
    int childIndex,
    PostCreationCallback postCreationCB)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_parents(parents)
    , m_childIndex(childIndex)
    , m_entries()
    , m_postCreationCB(postCreationCB)
{
    setText(QString("create element") + (m_parents.empty() ? "" : "s"));
}

void CommandHierarchyItemCreate::undo()
{
    UndoStackExecutionScope s(m_stack);

    HierarchyHelpers::Delete(m_hierarchy, m_entries);
}

void CommandHierarchyItemCreate::redo()
{
    UndoStackExecutionScope s(m_stack);

    if (m_entries.empty())
    {
        // This is the first call to redo().

        HierarchyItemRawPtrList items;

        for (auto& parentEntityId : m_parents)
        {
            // Find a unique name for the new element
            AZStd::string uniqueName;
            UiCanvasBus::EventResult(
                uniqueName,
                m_hierarchy->GetEditorWindow()->GetCanvas(),
                &UiCanvasBus::Events::GetUniqueChildName,
                parentEntityId,
                UICANVASEDITOR_ELEMENT_NAME_DEFAULT,
                nullptr);

            // Create a new hierarchy item which will create a new entity
            QTreeWidgetItem* parent = HierarchyHelpers::ElementToItem(m_hierarchy, parentEntityId, true);
            AZ_Assert(parent, "No parent widget item found for parent entity");
            HierarchyItem* hierarchyItem = new HierarchyItem(m_hierarchy->GetEditorWindow(),
                *parent,
                m_childIndex,
                QString(uniqueName.c_str()),
                nullptr);
            items.push_back(hierarchyItem);

            AZ::Entity* element = items.back()->GetElement();
            m_postCreationCB(element);
        }

        // true: Put the serialized data in m_undoXml.
        HierarchyClipboard::Serialize(m_hierarchy, m_hierarchy->selectedItems(), &items, m_entries, true);
        AZ_Assert(!m_entries.empty(), "Failed to serialize");
    }
    else
    {
        HierarchyHelpers::CreateItemsAndElements(m_hierarchy, m_entries);
    }

    HierarchyHelpers::ExpandParents(m_hierarchy, m_entries);

    m_hierarchy->clearSelection();
    HierarchyHelpers::SetSelectedItems(m_hierarchy, &m_entries);
}

void CommandHierarchyItemCreate::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    const QTreeWidgetItemRawPtrQList& selectedItems,
    int childIndex,
    PostCreationCallback postCreationCB)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemCreate(stack,
            hierarchy,
            SelectionHelpers::GetSelectedElementIds(hierarchy,
                selectedItems,
                true
                ),
            childIndex,
            postCreationCB));
}
