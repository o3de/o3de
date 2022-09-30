/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/SortAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    RowSortAdapter::~RowSortAdapter()
    {
    }

    void RowSortAdapter::SetSortActive(bool active)
    {
        if (m_sortActive != active)
        {
            m_sortActive = active;
            if (active)
            {
                // TODO: This method should genenerate patches to/from source once custom patching is implemented
                HandleReset();

                // we can use the currently cached tree, if it still exists. NB it will be cleared by DOM changes
                if (!m_rootNode)
                {
                    // TODO: GenerateFullTree();
                }
                // TODO: generate RemovalsFromSource patch
            }
            else
            {
                // TODO: generate PatchToSource patch
            }
        }
    }

    Dom::Value RowSortAdapter::GenerateContents()
    {
        if (!m_sourceAdapter)
        {
            return Dom::Value();
        }

        auto filteredContents = m_sourceAdapter->GetContents();
        if (m_sortActive)
        {
            filteredContents = GetSortedValue(filteredContents, m_rootNode.get());
        }
        return filteredContents;
    }

    void RowSortAdapter::HandleReset()
    {
        m_rootNode.reset();
        if (m_sortActive)
        {
            GenerateFullTree();
        }
        NotifyResetDocument(DocumentResetType::HardReset);
    }

    void RowSortAdapter::HandleDomChange([[maybe_unused]] const Dom::Patch& patch)
    {
        if (m_sortActive)
        {
            // TODO: handle changes upstream by making  custom patches. This is not currently
            // possible, because a sort change is mostly move operations, and the DPE doesn't handle moves yet.
            HandleReset();
        }
        else
        {
            // dom has changed while sort is disabled, destroy the cache
            m_rootNode.reset();
        }
    }

    void RowSortAdapter::InvalidateSort()
    {
        // TODO: once the DPE supports move patch operations, generate a patch here instead
        HandleReset();
    }

    void RowSortAdapter::GenerateFullTree()
    {
        const auto& sourceContents = m_sourceAdapter->GetContents();
        m_rootNode = AZStd::unique_ptr<SortInfoNode>(NewSortInfoNode());
        PopulateChildren(sourceContents, m_rootNode.get());
    }

    void RowSortAdapter::PopulateChildren(const Dom::Value& value, SortInfoNode* sortInfo)
    {
        AZ_Assert(sortInfo->m_indexSortedChildren.empty(), "PopulateChildren called on non-empty node!");
        for (size_t childIndex = 0, numChildren = value.ArraySize(); childIndex < numChildren; ++childIndex)
        {
            const auto& childValue = value[childIndex];

            // only cache/sort row entries. All other values will be left alone by the sort
            if (IsRow(childValue))
            {
                auto newChild = NewSortInfoNode();
                newChild->m_domIndex = childIndex;
                CacheDomInfoForNode(childValue, newChild);

                // insert into both lists, one in index order, and the other ordered by the adapter's lessThan function
                sortInfo->m_indexSortedChildren.insert(AZStd::unique_ptr<SortInfoNode>(newChild));
                sortInfo->m_adapterSortedChildren.insert(newChild);

                // recurse to children
                PopulateChildren(childValue, newChild);
            }
        }
    }

    Dom::Value RowSortAdapter::GetSortedValue(const Dom::Value& sourceValue, SortInfoNode* sortInfo)
    {
        // copy the entire Dom::Value, but this is cheap, thanks to its copy-on-write semantics
        auto sortedValue = sourceValue;

        AZ_Assert(sortInfo->m_indexSortedChildren.size() == sortInfo->m_adapterSortedChildren.size(), "child lists must be the same size!");

        auto nextSortedIter = sortInfo->m_adapterSortedChildren.begin();
        for (auto nextIndexIter = sortInfo->m_indexSortedChildren.begin(), endIter = sortInfo->m_indexSortedChildren.end();
             nextIndexIter != endIter;
             ++nextIndexIter, ++nextSortedIter)
        {
            size_t nextIndex = (*nextIndexIter)->m_domIndex;
            size_t nextInSort = (*nextSortedIter)->m_domIndex;

            // sort the source child before copying it
            auto sortedChild = GetSortedValue(sourceValue[nextInSort], *nextSortedIter);
            sortedValue[nextIndex] = sortedChild;
        }

        return sortedValue;
    }

} // namespace AZ::DocumentPropertyEditor
