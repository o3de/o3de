/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/ranges/zip_view.h>
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

            Dom::Patch outgoingPatch;
            if (active)
            {
                // we can use the currently cached tree, if it still exists. NB it will be cleared by DOM changes
                if (!m_rootNode)
                {
                    GenerateFullTree();
                }
            }
            else if (!m_rootNode)
            {
                // tree wasn't generated yet and sort is being disabled, nothing to do.
                // Wait for the normal GenerateContents to be called to generate the initial tree
                return;
            }
            GenerateMovePatches(m_rootNode.get(), Dom::Path(), !active, outgoingPatch);
            if (outgoingPatch.size())
            {
                NotifyContentsChanged(outgoingPatch);
            }
        }
    }

    Dom::Path RowSortAdapter::MapPath(const Dom::Path& path, bool mapFromSource) const
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
                for (auto [indexSortedNode, adapterSortedNode] :
                     AZStd::views::zip(currNode->m_indexSortedChildren, currNode->m_adapterSortedChildren))
                {
                    const SortInfoNode* comparisonNode =
                        static_cast<SortInfoNode*>((mapFromSource ? adapterSortedNode : indexSortedNode.get()));
                    const auto comparisonIndex = comparisonNode->m_domIndex;
                    const auto pathIndex = pathEntry.GetIndex();
                    if (comparisonIndex == pathIndex)
                    {
                        // set the mapped entry
                        const auto mappedIndex = (mapFromSource ? indexSortedNode->m_domIndex : adapterSortedNode->m_domIndex);
                        mappedPath.Push(mappedIndex);
                        currNode = comparisonNode;
                        found = true;
                        break;
                    }
                    else if (!mapFromSource && pathIndex < comparisonIndex)
                    {
                        // if we're searching the index-ordered container, we can stop if we've passed the index we're looking for
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

    bool RowSortAdapter::LessThan(SortInfoNode* lhs, SortInfoNode* rhs) const
    {
        return (lhs->m_domIndex < rhs->m_domIndex);
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
                        ResortRow(GetRowPath(patchPath), outgoingPatch);
                    }
                    else
                    {
                        needsReset = true;
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
                // TODO: handle other patch types here
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

    RowSortAdapter::SortInfoNode* RowSortAdapter::GetNodeAtSourcePath(Dom::Path sourcePath)
    {
        SortInfoNode* currNode = m_rootNode.get();
        for (auto pathIter = sourcePath.begin(), endIter = sourcePath.end(); pathIter != endIter; ++pathIter)
        {
            if (!pathIter->IsIndex())
            {
                return nullptr;
            }
            auto searchChild = AZStd::make_unique<SortInfoBase>(pathIter->GetIndex());
            auto foundChild = currNode->m_indexSortedChildren.find(searchChild);
            if (foundChild == currNode->m_indexSortedChildren.end())
            {
                return nullptr;
            }
            currNode = static_cast<SortInfoNode*>(foundChild->get());
        }
        return currNode;
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
                newChild->m_parentNode = sortInfo;
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

    void RowSortAdapter::ResortRow(Dom::Path sourcePath, Dom::Patch& outgoingPatch)
    {
        // get the actual node at sortedPath
        auto* node = GetNodeAtSourcePath(sourcePath);

        // remove the (possibly changed) node from its sorted set and re-add it to see if it ends up in the same position
        auto& parentNode = node->m_parentNode;
        auto& sortedSet = parentNode->m_adapterSortedChildren;
        auto sortedIter = sortedSet.find(node);
        AZ_Assert(sortedIter != sortedSet.end(), "node not found in sorted set!");
        sortedSet.erase(sortedIter);
        auto insertedIter = sortedSet.insert(node).first;
        auto indexIter = parentNode->m_indexSortedChildren.begin();
        sortedIter = sortedSet.begin();

        // walk both sets until we get to the newly inserted node
        while (sortedIter != insertedIter && sortedIter != sortedSet.end())
        {
            ++sortedIter;
            ++indexIter;
        }
        AZ_Assert(sortedIter != sortedSet.end(), "re-inserted node not found!");
        const auto newIndex = (*indexIter)->m_domIndex;

        // record row's existing sorted index for comparison
        auto sortedPath = MapFromSourcePath(sourcePath);
        const auto oldIndex = sortedPath.Back().GetIndex();
        if (newIndex != oldIndex)
        {
            auto newPath = sortedPath;
            newPath.Pop();
            newPath.Push(newIndex);
            outgoingPatch.PushBack(Dom::PatchOperation::MoveOperation(newPath, sortedPath));
        }
    }

    void RowSortAdapter::GenerateMovePatches(
        const SortInfoNode* sortNode, Dom::Path parentPath, bool mapToSource, Dom::Patch& outgoingPatch)
    {
        if (sortNode->m_indexSortedChildren.empty())
        {
            // no children, nothing to do
            return;
        }

        AZStd::unordered_map<size_t, size_t> sourceToDestination;
        auto firstMoveIndex = outgoingPatch.size();

        auto zippedView = AZStd::views::zip(sortNode->m_indexSortedChildren, sortNode->m_adapterSortedChildren);
        // generate a map of child move patch operations for this parent
        for (auto [indexSortedNode, adapterSortedNode] : zippedView)
        {
            size_t sourceIndex, destinationIndex;
            if (mapToSource)
            {
                destinationIndex = adapterSortedNode->m_domIndex;
                sourceIndex = indexSortedNode->m_domIndex;
            }
            else
            {
                destinationIndex = indexSortedNode->m_domIndex;
                sourceIndex = adapterSortedNode->m_domIndex;
            }
            sourceToDestination[sourceIndex] = destinationIndex;
        }

        // iterate over the mappings, adjusting the source index by any prior moves that have crossed it
        auto currMapping = sourceToDestination.extract(sourceToDestination.begin());
        while (!currMapping.empty())
        {
            auto& sourceIndex = currMapping.key();
            auto& destinationIndex = currMapping.mapped();
            for (auto operationIndex = firstMoveIndex, numMoves = outgoingPatch.size(); operationIndex < numMoves; ++operationIndex)
            {
                auto& currMove = outgoingPatch[operationIndex];
                const auto operationSource = currMove.GetSourcePath().Back().GetIndex();
                const auto operationDest = currMove.GetDestinationPath().Back().GetIndex();

                if (operationSource > operationDest)
                {
                    // check if this prior operation crossed us moving backwards which would've incremented our source index
                    if (operationSource > sourceIndex && operationDest <= sourceIndex)
                    {
                        ++sourceIndex;
                    }
                }
                else
                {
                    // check if this prior operation crossed us moving forwards which would've decremented our source index
                    if (operationSource < sourceIndex && operationDest >= sourceIndex)
                    {
                        --sourceIndex;
                    }
                }
            }

            if (sourceIndex != destinationIndex)
            {
                // it's an actual different index, generate the move patch
                auto destPath = parentPath / destinationIndex;
                auto sourcePath = parentPath / sourceIndex;
                outgoingPatch.PushBack(Dom::PatchOperation::MoveOperation(destPath, sourcePath));
            }

            // next do the mapping whose source is the previous operation's destination
            auto nextMapping = sourceToDestination.find(destinationIndex);
            if (nextMapping == sourceToDestination.end())
            {
                // there's no mapping starting there - we've reached the end of a loop
                if (sourceToDestination.empty())
                {
                    // no remaining mappings, we're done!
                    currMapping = {};
                }
                else
                {
                    // we've reached the end of a loop, but there are more mappings to account for
                    // Grab one from the front and start a new loop
                    // because this loop is finished, no mappings are displaced, we can reset firstMoveIndex
                    firstMoveIndex = outgoingPatch.size();
                    currMapping = sourceToDestination.extract(sourceToDestination.begin());
                }
            }
            else
            {
                currMapping = sourceToDestination.extract(nextMapping);
            }
        }

        // Now loop through the children again so that they can generate their children's patches. Note that we can't recurse into children
        // until all moves for this level are done, because the move operations for this level should be adjacent in the patch for searching
        for (auto [indexSortedNode, adapterSortedNode] : zippedView)
        {
            // now recurse to children, note that we're always mapping to the indexSortedNode dom index,
            // whether it's mapping the sorted node to its new place or mapping the index node back to its home
            SortInfoNode* sourceNode = (mapToSource ? static_cast<SortInfoNode*>(indexSortedNode.get()) : adapterSortedNode);
            GenerateMovePatches(sourceNode, parentPath / indexSortedNode->m_domIndex, mapToSource, outgoingPatch);
        }
    }
} // namespace AZ::DocumentPropertyEditor
