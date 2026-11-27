/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Helpers/EntityHelpers.h"
#include "Widgets/HierarchyWidget/HierarchyItem.h"
#include "Widgets/HierarchyWidget/TreeWidgetItemList.h"

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/list.h>
#include <QUndoCommand>

class HierarchyWidget;
class UndoStack;

class CommandHierarchyItemReparent : public QUndoCommand
{
public:
    void undo() override;
    void redo() override;

    static void Push(
        UndoStack* stack, HierarchyWidget* hierarchy, const HierarchyItemRawPtrList& items, const QTreeWidgetItemRawPtrList& itemParents);

private:
    CommandHierarchyItemReparent(UndoStack* stack, HierarchyWidget* hierarchy, const HierarchyItemRawPtrList& items);

    struct ChildItem
    {
        AZ::EntityId m_id;
        AZ::EntityId m_parentId;
        int m_row;
    };

    using ChildItemList = AZStd::list<ChildItem>;

    void Reparent(ChildItemList& sourceChildren, ChildItemList& destChildren, bool& childrenSorted);

private:
    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;

    ChildItemList m_sourceChildren;
    ChildItemList m_destinationChildren;

    bool m_sourceChildrenSorted;
    bool m_destinationChildrenSorted;

    EntityHelpers::EntityIdList m_listToValidate;

    // The first execution of redo() is done in REACTION to a Qt
    // event that has ALREADY completed some necessary work. We ONLY
    // want to execute all of redo() on SUBSEQUENT calls.
    bool m_isFirstExecution;
};
