/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

namespace SelectionHelpers
{
    //-------------------------------------------------------------------------------

    void UnmarkAllAndAllTheirChildren(QTreeWidgetItem* baseRootItem)
    {
        // Seed the list.
        HierarchyItemRawPtrList itemList;
        HierarchyHelpers::AppendAllChildrenToEndOfList(baseRootItem, itemList);

        // Traverse the list.
        HierarchyHelpers::TraverseListAndAllChildren(itemList,
            [](HierarchyItem* childItem)
            {
                childItem->SetMark(false);
            });
    }

    void MarkOnlyDirectChildrenOf(const QTreeWidgetItemRawPtrQList& parentItems)
    {
        for (auto j : parentItems)
        {
            int end = j->childCount();
            for (int i = 0; i < end; ++i)
            {
                HierarchyItem* item = HierarchyItem::RttiCast(j->child(i));
                item->SetMark(true);
            }
        }
    }

    bool A_IsParentOf_B(QTreeWidgetItem* A,
        QTreeWidgetItem* B)
    {
        // We MUST have an "A", and "A" and "B" MUST be different.
        // Otherwise, early-out. We don't need to test "B" here,
        // by itself, because we'll do so in the "while" loop below.
        if (!(A && (A != B)))
        {
            // Nothing to do.
            return false;
        }

        while (B)
        {
            // Walk up the hierarchy.
            B = B->parent();

            // If A is the parent of B,
            // A will eventually equal B.
            if (A == B)
            {
                return true;
            }
        }

        return false;
    }

    bool IsMarkedOrParentIsMarked(HierarchyItem* item)
    {
        if (!item)
        {
            // Nothing to do.
            return false;
        }

        do
        {
            if (item->GetMark())
            {
                return true;
            }

            // Walk up the hierarchy.
            item = item->Parent();
        }
        while (item);

        return false;
    }

    void FindUnmarked(HierarchyItemRawPtrList& results,
        const QTreeWidgetItemRawPtrQList& parentItems)
    {
        for (auto j : parentItems)
        {
            HierarchyItem* item = HierarchyItem::RttiCast(j);
            AZ_Assert(item, "There's an item in the Hierarchy that isn't a HierarchyItem.");

            if (!IsMarkedOrParentIsMarked(item))
            {
                results.push_back(item);
            }
        }
    }

    //-------------------------------------------------------------------------------

    AZ::Entity* GetTopLevelParentOfElement(const LyShine::EntityArray& elements, AZ::Entity* elementToFind)
    {
        do
        {
            for (auto e : elements)
            {
                if (e == elementToFind)
                {
                    return e;
                }
            }

            elementToFind = EntityHelpers::GetParentElement(elementToFind);
        } while (elementToFind);

        return nullptr;
    }

    void RemoveEntityFromArray(LyShine::EntityArray& listToTrim, const AZ::Entity* entityToRemove)
    {
        for (auto e = listToTrim.begin(); e != listToTrim.end(); ++e)
        {
            if (*e == entityToRemove)
            {
                listToTrim.erase(e);
                return;
            }
        }
    }

    //-------------------------------------------------------------------------------

    void GetListOfTopLevelSelectedItems([[maybe_unused]] const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        QTreeWidgetItemRawPtrQList& results)
    {
        AZ_Assert(&selectedItems != &results, "Input and output cannot be the same");

        results = selectedItems;

        // Remove all non-top-parent nodes.
        // IMPORTANT: This algorithm's time complexity is O(n^2),
        // and space complexity is O(n).
        for (auto i : selectedItems)
        {
            for (auto j : selectedItems)
            {
                if ((i != j) &&
                    A_IsParentOf_B(i, j))
                {
                    results.removeOne(j);
                }
            }
        }
    }

    void GetListOfTopLevelSelectedItems([[maybe_unused]] const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        QTreeWidgetItem* invisibleRootItem,
        HierarchyItemRawPtrList& results)
    {
        UnmarkAllAndAllTheirChildren(invisibleRootItem);

        // Note: The mark is used as a pruning flag.
        // All items with a mark, or under a marked item, will be culled.
        MarkOnlyDirectChildrenOf(selectedItems);

        FindUnmarked(results, selectedItems);
    }

    //-------------------------------------------------------------------------------

    HierarchyItemRawPtrList GetSelectedHierarchyItems([[maybe_unused]] const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems)
    {
        HierarchyItemRawPtrList items;

        // selectedItems -> HierarchyItemRawPtrList.
        for (auto i : selectedItems)
        {
            HierarchyItem* item = HierarchyItem::RttiCast(i);
            if (item)
            {
                items.push_back(item);
            }
            else
            {
                AZ_Assert(0, "This should NEVER happen. Because we should ONLY be able to select HierarchyItem in the widget.");
            }
        }

        return items;
    }

    LyShine::EntityArray GetSelectedElements([[maybe_unused]] const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems)
    {
        auto count = selectedItems.count();
        LyShine::EntityArray elements(count);
        {
            for (int i = 0; i < count; ++i)
            {
                HierarchyItem* item = HierarchyItem::RttiCast(selectedItems[i]);
                if (item)
                {
                    elements[i] = item->GetElement();
                }
                else
                {
                    AZ_Assert(0, "This should NEVER happen, because every item in the hierarchy should represent an element.");
                }
            }
        }

        return elements;
    }

    EntityHelpers::EntityIdList GetSelectedElementIds([[maybe_unused]] const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems,
        bool addInvalidIdIfEmpty)
    {
        EntityHelpers::EntityIdList ids;
        {
            auto count = selectedItems.count();

            for (int i = 0; i < count; ++i)
            {
                HierarchyItem* item = HierarchyItem::RttiCast(selectedItems[i]);
                if (item)
                {
                    ids.push_back(item->GetEntityId());
                }
                else
                {
                    AZ_Assert(0, "This should NEVER happen, because every item in the hierarchy should represent an element.");
                }
            }

            if (addInvalidIdIfEmpty && ids.empty())
            {
                ids.push_back(AZ::EntityId());
            }
        }

        return ids;
    }

    LyShine::EntityArray GetTopLevelSelectedElements(const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems)
    {
        HierarchyItemRawPtrList topLevelSelectedItems;
        GetListOfTopLevelSelectedItems(widget,
            selectedItems,
            widget->invisibleRootItem(),
            topLevelSelectedItems);

        // HierarchyItemRawPtrList -> EntityArray.
        LyShine::EntityArray elements;
        for (auto item : topLevelSelectedItems)
        {
            elements.push_back(item->GetElement());
        }

        return elements;
    }

    LyShine::EntityArray GetTopLevelSelectedElementsNotControlledByParent(const HierarchyWidget* widget,
        const QTreeWidgetItemRawPtrQList& selectedItems)
    {
        HierarchyItemRawPtrList topLevelSelectedItems;
        GetListOfTopLevelSelectedItems(widget,
            selectedItems,
            widget->invisibleRootItem(),
            topLevelSelectedItems);

        // HierarchyItemRawPtrList -> EntityArray.
        LyShine::EntityArray elements;
        for (auto item : topLevelSelectedItems)
        {
            if (!ViewportHelpers::IsControlledByLayout(item->GetElement()))
            {
                elements.push_back(item->GetElement());
            }
        }

        return elements;
    }

    //-------------------------------------------------------------------------------
}   // namespace SelectionHelpers
