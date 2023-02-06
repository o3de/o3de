/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/SortAdapter.h>
#include <AzCore/std/ranges/zip_view.h>

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
                // TODO: This method should generate patches to/from source once custom patching is implemented
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

    Dom::Path RowSortAdapter::MapPath(const Dom::Path& path, bool mapToSource) const
    {
        if (!m_sortActive)
        {
            return path;
        }

        Dom::Path mappedPath;
        const auto* currNode = m_rootNode.get();

        // go through each entry in the path, looking for an existing mapping at the give index
        for (const auto& pathEntry : path)
        {
            if (pathEntry.IsIndex())
            {
                bool found = false;
                for (auto [indexSortedNode, adapterSortedNode] : AZStd::views::zip(currNode->m_indexSortedChildren, currNode->m_adapterSortedChildren))
                {
                    const SortInfoNode* comparisonNode = (mapToSource ? adapterSortedNode : indexSortedNode.get());
                    if (comparisonNode->m_domIndex == pathEntry.GetIndex())
                    {
                        // set the mapped entry
                        const auto mappedIndex = (mapToSource ? indexSortedNode->m_domIndex : adapterSortedNode->m_domIndex);
                        mappedPath.Push(mappedIndex);
                        currNode = comparisonNode;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    // if there's no mapping, this *should* be a column entry. Pass it through as-is
                    mappedPath.Push(pathEntry);
                }
            }
            else
            {
                // RowSortAdapter only affect index entries, pass other types (like attribute strings) through
                mappedPath.Push(pathEntry);
            }
        }
        return mappedPath;
    }

    Dom::Path RowSortAdapter::MapFromSourcePath(const Dom::Path& sourcePath) const
    {
        return MapPath(sourcePath, true);
    }

    Dom::Path RowSortAdapter::MapToSourcePath(const Dom::Path& filterPath) const
    {
        return MapPath(filterPath, false);
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
        NotifyResetDocument();
    }

    void RowSortAdapter::HandleDomChange(const Dom::Patch& patch)
    {
        if (m_sortActive)
        {
            bool needsReset = false;

            Dom::Patch outgoingPatch;
            for (auto operationIterator = patch.begin(), endIterator = patch.end(); !needsReset && operationIterator != endIterator;
                 ++operationIterator)
            {
                const auto& patchPath = operationIterator->GetDestinationPath();
                if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
                {
                    needsReset = true;
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
                {
                    if (!IsRow(patchPath))
                    {
                        // pass on the replace column operation with the mapped path
                        auto sortedPath = MapFromSourcePath(patchPath);
                        outgoingPatch.PushBack(Dom::PatchOperation::ReplaceOperation(sortedPath, operationIterator->GetValue()));

                        // the replaced column entry might've changed the row's sorting. Updated it
                        sortedPath.Pop();
                        ResortRow(sortedPath);
                    }
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
                {
                    needsReset = true;
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Move)
                {
                    needsReset = true;
                }
            }

            if (needsReset)
            {
                // TODO: handle other patch types here. Some of this is not currently possible, 
                // because a sort change is mostly move operations, and the DPE doesn't handle moves yet.
                HandleReset();
            }
            else if (outgoingPatch.Size())
            {
                NotifyContentsChanged(outgoingPatch);
            }
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
        // copy the entire Dom::Value and sort in place. Note that this is cheap, thanks to its copy-on-write semantics
        auto sortedValue = sourceValue;

        AZ_Assert(sortInfo->m_indexSortedChildren.size() == sortInfo->m_adapterSortedChildren.size(), "child lists must be the same size!");

        // iterate children in model index order, copying from the adapter sorted value into the current position
        auto nextSortedIter = sortInfo->m_adapterSortedChildren.begin();
        for (auto nextIndexIter = sortInfo->m_indexSortedChildren.begin(), endIter = sortInfo->m_indexSortedChildren.end();
             nextIndexIter != endIter;
             ++nextIndexIter, ++nextSortedIter)
        {
            size_t nextIndex = (*nextIndexIter)->m_domIndex;
            size_t nextInSort = (*nextSortedIter)->m_domIndex;

            // sort the source child before copying it
            auto sortedChild = GetSortedValue(sourceValue[nextInSort], *nextSortedIter);
            
            // do the actual copy into the current position
            sortedValue[nextIndex] = sortedChild;
        }

        return sortedValue;
    }

    void RowSortAdapter::ResortRow(Dom::Path sortedPath)
    {
        // TODO once the DPE supports move operations:
        // record row's sorted index, remove it, re-cache its data,add it, check its new index. If changed, generate move operation
        // NB: this isn't necessary for flat adapters that can't be re-ordered, so the CVarAdapter doesn't need this
    }

} // namespace AZ::DocumentPropertyEditor
