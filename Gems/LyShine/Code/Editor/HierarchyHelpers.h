/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorWindow.h"
#include "HierarchyWidget.h"
#include "EditorCommon.h"

namespace HierarchyHelpers
{
    //-------------------------------------------------------------------------------

    void Delete(HierarchyWidget* hierarchy,
        SerializeHelpers::SerializedEntryList& entries);

    template< typename T >
    void AppendAllChildrenToEndOfList(QTreeWidgetItem* rootItem,
        T& itemList);

    //! Handles whether the given item should be de-selected for control-key multi-selection.
    //
    //! \return True if the item has been de-selected, false otherwise.
    bool HandleDeselect(QTreeWidgetItem* widgetItem,
        const bool controlKeyPressed);

    //-------------------------------------------------------------------------------

    QAction* CreateAddElementAction(HierarchyWidget* hierarchy,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        bool addAtRoot,
        const QPoint* optionalPos);

    //-------------------------------------------------------------------------------

    void CreateItemsAndElements(HierarchyWidget* widget,
        const SerializeHelpers::SerializedEntryList& entryList);

    using Creator = std::function<void( HierarchyItem* parent,
                                        LyShine::EntityArray& listOfNewlyCreatedTopLevelElements )>;

    LyShine::EntityArray CreateItemsAndElements(HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        bool createAsChildOfSelection,
        Creator creator);

    //-------------------------------------------------------------------------------

    QTreeWidgetItem* ElementToItem(HierarchyWidget* widget, const AZ::Entity* element, bool defaultToInvisibleRootItem);
    QTreeWidgetItem* ElementToItem(HierarchyWidget* widget, AZ::EntityId elementId, bool defaultToInvisibleRootItem);

    //-------------------------------------------------------------------------------

    //! These are helper functions for the templated functions.
    //! These AREN'T meant to be used outside of the scope of this file.

    QTreeWidgetItem* _GetItem(HierarchyWidget* widget,
        QTreeWidgetItem* i);

    QTreeWidgetItem* _GetItem(HierarchyWidget* widget,
        HierarchyItem* i);

    QTreeWidgetItem* _GetItem(HierarchyWidget* widget,
        SerializeHelpers::SerializedEntry& e);

    QTreeWidgetItem* _GetItem(HierarchyWidget* widget,
        AZ::Entity* e);

    QTreeWidgetItem* _GetItem(HierarchyWidget* widget,
        AZ::EntityId e);

    //-------------------------------------------------------------------------------

    template< typename T >
    bool AllItemExists(HierarchyWidget* widget,
        T& listToValidate)
    {
        QTreeWidgetItem* rootItem = widget->invisibleRootItem();

        QTreeWidgetItemRawPtrList allItems;
        HierarchyHelpers::AppendAllChildrenToEndOfList(rootItem, allItems);

        // IMPORTANT: If the items in each list were sorted, we could use
        // std::set_difference() or std::set_intersection() instead.
        for (auto && p : listToValidate)
        {
            QTreeWidgetItem* item = _GetItem(widget, p);

            if (item == rootItem)
            {
                // The invisibleRootItem is valid.
                continue;
            }

            if (std::find(allItems.begin(), allItems.end(), item) == allItems.end())
            {
                // Not found.
                return false;
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------------

    //! Constructs a single-element array for the given element and forwards the request to SetSelectedItems
    //
    //! If the given element is null, this function is equivalent to calling SetSelectedItems
    //! with an empty list (and effectively clears the selection for the given widget).
    //
    //! \sa SetSelectedItems
    void SetSelectedItem(HierarchyWidget* widget,
        AZ::Entity* element);

    //! Selects all items in the given list and sets the first item in the list as the current item, or clears the current selection if the list is empty.
    //
    //! This is the list-based counterpart to SetSelectedItem.
    //
    //! All items in the given list will be selected for the given widget, unless
    //! the list is null or empty, in which case the current selection for the
    //! widget will be cleared.
    //
    //! \param widget The HierarchyWidget to operate on
    //
    //! \param list A list of elements to select
    //
    //! \sa SetSelectedItem
    template< typename T >
    void SetSelectedItems(HierarchyWidget* widget,
        T* list)
    {
        // This sets the selected item AND the current item.
        // Qt is smart enough to recognize and handle multi-selection
        // properly when the Ctrl key or the Shift key is pressed.

        // Stop object pick mode when an action explicitly wants to set the hierarchy's selected items
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        if ((!list) || list->empty())
        {
            // Note that calling SetSelectedItems with an empty list should clear
            // the selection of the tree widget
            widget->clearSelection();

            return;
        }

        AZ_Assert(
            widget->selectionMode() == QAbstractItemView::ExtendedSelection,
            "If this assert has triggered, then our selection mode assumptions have changed! "
            "Changing the selection mode could cause bugs and needs QA verification.");

        // The "current item" is like a cursor.
        // It's valid to have multiple items selected,
        // but you can only have ONE "current item".

        for (auto && i : *list)
        {
            QTreeWidgetItem* item = _GetItem(widget, i);

            // item can be null in the case of restoring a selection when switching tabs and an entity in a slice has been deleted
            if (item)
            {
                if (item == _GetItem(widget, list->front()))
                {
                    // Calling setCurrentItem will set the item as current, but won't
                    // necessarily select the item. If the item is already selected,
                    // and the control or shift key is pressed, the item will
                    // actually become de-selected (we explicitly set the selected
                    // property below to ensure the item becomes selected).
                    widget->setCurrentItem(item);
                }

                item->setSelected(true);
            }
        }
    }

    //-------------------------------------------------------------------------------

    template< typename T >
    void ExpandParents(HierarchyWidget* widget,
        T& items)
    {
        for (auto && i : items)
        {
            QTreeWidgetItem* p = _GetItem(widget, i)->QTreeWidgetItem::parent();
            if (p)
            {
                p->setExpanded(true);
            }
        }
    }

    //-------------------------------------------------------------------------------

    template< typename T >
    void ExpandItemsAndAncestors(HierarchyWidget* widget,
        T& items)
    {
        for (auto && i : items)
        {
            QTreeWidgetItem* item = _GetItem(widget, i);
            
            // Expand parent first, then child
            QTreeWidgetItemRawPtrList itemsToExpand;
            while (item)
            {
                itemsToExpand.push_front(item);
                item = item->parent();
            }
            for (auto itemToExpand : itemsToExpand)
            {
                itemToExpand->setExpanded(true);
            }
        }
    }

    //-------------------------------------------------------------------------------

    template< typename T >
    void AppendAllChildrenToEndOfList(QTreeWidgetItem* rootItem,
        T& itemList)
    {
        // Seed the list.
        int end = rootItem->childCount();
        for (int i = 0; i < end; ++i)
        {
            itemList.push_back(static_cast<typename T::value_type>(rootItem->child(i)));
        }

        // Note: This is a breadth-first traversal through all items.
        for (auto item : itemList)
        {
            int itemEnd = item->childCount();

            for (int i = 0; i < itemEnd; ++i)
            {
                itemList.push_back(static_cast<typename T::value_type>(item->child(i)));
            }
        }
    }

    //-------------------------------------------------------------------------------

    //! Important: This function has the side-effect of modifying the input list ("rootList").
    template< typename T >
    void TraverseListAndAllChildren(T& rootList,
        std::function<void(typename T::value_type)> traverser)
    {
        // Note: This is a breadth-first traversal through all items.
        for (auto item : rootList)
        {
            traverser(item);

            int end = item->childCount();

            for (int i = 0; i < end; ++i)
            {
                rootList.push_back(static_cast<typename T::value_type>(item->child(i)));
            }
        }
    }

    //! returns true if item1 is before item2 in the element hierarchy
    bool CompareOrderInElementHierarchy(HierarchyItem* item1, HierarchyItem* item2);

    //! Sort the given list so that the items is in the order that they appear in the element hierarchy
    void SortByHierarchyOrder(HierarchyItemRawPtrList &itemsToSerialize);

    //-------------------------------------------------------------------------------
}   // namespace HierarchyHelpers
