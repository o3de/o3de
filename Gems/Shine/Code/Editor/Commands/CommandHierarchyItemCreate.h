/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Helpers/EntityHelpers.h"
#include "Helpers/SerializeHelpers.h"
#include "Widgets/HierarchyWidget/TreeWidgetItemList.h"

#include <AzCore/std/functional.h>

#include <QUndoCommand>

class UndoStack;
class HierarchyWidget;

namespace AZ
{
    class Entity;
}

class CommandHierarchyItemCreate : public QUndoCommand
{
public:
    using PostCreationCallback = AZStd::function<void(AZ::Entity* element)>;

    void undo() override;
    void redo() override;

    static void Push(
        UndoStack* stack,
        HierarchyWidget* hierarchy,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        int childIndex = -1,
        PostCreationCallback postCreationCB =
            []([[maybe_unused]] AZ::Entity* element)
        {
        });

private:
    CommandHierarchyItemCreate(
        UndoStack* stack,
        HierarchyWidget* hierarchy,
        const EntityHelpers::EntityIdList& parents,
        int childIndex,
        PostCreationCallback postCreationCB);

private:
    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;
    EntityHelpers::EntityIdList m_parents;
    int m_childIndex;
    SerializeHelpers::SerializedEntryList m_entries;
    PostCreationCallback m_postCreationCB;
};
