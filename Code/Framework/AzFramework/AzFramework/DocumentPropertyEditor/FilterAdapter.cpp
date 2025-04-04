/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/DocumentPropertyEditor/FilterAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    RowFilterAdapter::RowFilterAdapter()
    {
    }

    RowFilterAdapter::~RowFilterAdapter()
    {
        delete m_root;
    }

    void RowFilterAdapter::SetIncludeAllMatchDescendants(bool includeAll)
    {
        m_includeAllMatchDescendants = includeAll;
        UpdateMatchDescendants(m_root);
    }

    bool RowFilterAdapter::FilterIsActive() const
    {
        return m_filterActive;
    }

    Dom::Path RowFilterAdapter::MapFromSourcePath(const Dom::Path& sourcePath) const
    {
        if (!m_filterActive)
        {
            return sourcePath;
        }
        Dom::Path filterPath;
        MatchInfoNode* currNode = m_root;

        for (const auto& pathEntry : sourcePath)
        {
            if (!pathEntry.IsIndex())
            {
                // RowFilterAdapter only affects index entries, pass other types through
                filterPath.Push(pathEntry);
            }
            else
            {
                const auto targetIndex = pathEntry.GetIndex();
                const auto childCount = currNode->m_childMatchState.size();

                if (targetIndex >= childCount)
                {
                    AZ_Warning("FilterAdapter", false, "Invalid sourcePath path sent to MapFromSourcePath!");
                    return Dom::Path();
                }
                else if (currNode->m_childMatchState[targetIndex] && !currNode->m_childMatchState[targetIndex]->IncludeInFilter())
                {
                    // the entry at this index is a filtered out row, there is no mapped path
                    return Dom::Path();
                }
                else
                {
                    // iterate through each sourceIndex keeping track of what the next mapped filterIndex would be
                    size_t filterIndex = 0;
                    for (size_t sourceIndex = 0; sourceIndex < targetIndex; ++sourceIndex)
                    {
                        const auto currChildState = currNode->m_childMatchState[sourceIndex];
                        // skip rows that don't match; count null nodes and matching nodes
                        if (!currChildState || currChildState->IncludeInFilter())
                        {
                            ++filterIndex;
                        }
                    }

                    // record the filterIndex at this pathEntry and continue deeper
                    filterPath.Push(filterIndex);
                    currNode = currNode->m_childMatchState[targetIndex];
                }
            }
        }
        return filterPath;
    }

    Dom::Path RowFilterAdapter::MapToSourcePath(const Dom::Path& filterPath) const
    {
        if (!m_filterActive)
        {
            return filterPath;
        }

        Dom::Path sourcePath;
        MatchInfoNode* currNode = m_root;

        for (const auto& pathEntry : filterPath)
        {
            if (!pathEntry.IsIndex())
            {
                // RowFilterAdapter only affects index entries, pass other types through
                sourcePath.Push(pathEntry);
            }
            else
            {
                const auto targetIndex = pathEntry.GetIndex();
                const auto childCount = currNode->m_childMatchState.size();
                size_t filterIndex = 0;

                for (size_t sourceIndex = 0; sourceIndex < childCount; ++sourceIndex)
                {
                    // skip rows that don't match; count null nodes and matching nodes
                    if (!currNode->m_childMatchState[sourceIndex] || currNode->m_childMatchState[sourceIndex]->IncludeInFilter())
                    {
                        ++filterIndex;
                        if (filterIndex == targetIndex)
                        {
                            sourcePath.Push(sourceIndex);
                            currNode = currNode->m_childMatchState[sourceIndex];
                            break;
                        }
                    }
                }
                if (filterIndex != targetIndex)
                {
                    // if we ran out of children before we hit the desired filterIndex, the passed filterPath was invalid
                    AZ_Warning("FilterAdapter", false, "Invalid filter path sent to MapToSourcePath!");
                    return Dom::Path();
                }
            }
        }
        return sourcePath;
    }

    RowFilterAdapter::MatchInfoNode* RowFilterAdapter::MakeNewNode(RowFilterAdapter::MatchInfoNode* parentNode, unsigned int creationFrame)
    {
        MatchInfoNode* newNode = NewMatchInfoNode();
        newNode->m_parentNode = parentNode;
        newNode->m_lastFilterUpdateFrame = creationFrame;
        return newNode;
    }

    void RowFilterAdapter::SetFilterActive(bool activateFilter)
    {
        if (m_filterActive != activateFilter)
        {
            AZ::Dom::Patch patchToFill;
            m_filterActive = activateFilter;
            if (activateFilter)
            {
                // update entire tree if it exists, build it if it doesn't
                if (m_root)
                {
                    UpdateMatchDescendants(m_root);
                }
                else
                {
                    GenerateFullTree();
                }
                // generate a patch that consists of all the removals from the source adapter to get to the current filter state
                GeneratePatch(RemovalsFromSource, patchToFill);
            }
            else
            {
                // generate a patch that adds in any filtered out nodes, but leave tree alone in case the filter gets reactivated
                GeneratePatch(PatchToSource, patchToFill);
            }
            if (patchToFill.Size())
            {
                NotifyContentsChanged(patchToFill);
            }
        }
    }

    void RowFilterAdapter::InvalidateFilter()
    {
        AZ_Assert(m_filterActive, "InvalidateFilter is meaningless if the filter is inactive! (m_filterActive == false)");
        if (m_filterActive)
        {
            ++m_updateFrame;
            UpdateMatchDescendants(m_root);

            AZ::Dom::Patch patchToFill;
            GeneratePatch(Incremental, patchToFill);

            if (patchToFill.Size())
            {
                NotifyContentsChanged(patchToFill);
            }
        }
    }

    Dom::Value RowFilterAdapter::GenerateContents()
    {
        if (!m_sourceAdapter)
        {
            return Dom::Value();
        }

        auto filteredContents = m_sourceAdapter->GetContents();
        if (m_filterActive)
        {
            // filter downwards from root
            CullUnmatchedChildRows(filteredContents, m_root);
        }
        return filteredContents;
    }

    void RowFilterAdapter::HandleReset()
    {
        ++m_updateFrame;
        delete m_root;
        m_root = nullptr;

        if (m_filterActive)
        {
            GenerateFullTree();
        }
        NotifyResetDocument(DocumentResetType::HardReset);
    }

    void RowFilterAdapter::HandleDomChange(const Dom::Patch& patch)
    {
        ++m_updateFrame;
        Dom::Patch outgoingPatch;
        if (m_filterActive)
        {
            const auto& sourceContents = m_sourceAdapter->GetContents();
            AZStd::unordered_set<MatchInfoNode*> nodesToUpdate;
            for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
            {
                const auto& patchPath = operationIterator->GetDestinationPath();
                if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
                {
                    // generate patch operation for this removal in our address space
                    auto mappedPath = MapFromSourcePath(patchPath);
                    if (!mappedPath.IsEmpty())
                    {
                        outgoingPatch.PushBack(Dom::PatchOperation::RemoveOperation(mappedPath));
                    }

                    auto owningRowPath = GetRowPath(patchPath);
                    auto owningRow = GetMatchNodeAtPath(owningRowPath);
                    
                    // if the row path and patch path are the same, the removed item was a row
                    if (patchPath == owningRowPath)
                    {
                        // Patch says row is being removed, we need to remove it from our graph
                        auto& lastPathEntry = *(patchPath.end() - 1);
                        AZ_Assert(lastPathEntry.IsIndex(), "this removal entry must be an index!");
                        owningRow->m_parentNode->RemoveChildAtIndex(lastPathEntry.GetIndex());

                        // update former parent's match state, as its m_matchingDescendants may have changed
                        nodesToUpdate.insert(owningRow->m_parentNode);
                    }
                    else
                    {
                        // removed item wasn't a row, so we need to re-cache the owning row's cached info
                        nodesToUpdate.insert(owningRow);
                    }
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
                {
                    auto rowPath = GetRowPath(patchPath);

                    // replace operations can change child values from or into rows. Handle all cases
                    auto existingRowNode = GetMatchNodeAtPath(patchPath);
                    const bool replacementIsRow = (patchPath == rowPath);
                    if (!replacementIsRow && !existingRowNode)
                    {
                        // neither the value being replaced nor its replacement is a row. Map the path and pass it on
                        auto mappedPath = MapFromSourcePath(patchPath);
                        if (!mappedPath.IsEmpty())
                        {
                            outgoingPatch.PushBack(Dom::PatchOperation::ReplaceOperation(mappedPath, sourceContents[patchPath]));
                        }
                    }
                    else
                    {
                        // either the replacing value or the value being replaced is a row
                        // treat this as a remove operation with the possibility of a corresponding
                        // add operation, if the new value matches the filter
                        auto mappedPath = MapFromSourcePath(patchPath);
                        if (!mappedPath.IsEmpty())
                        {
                            outgoingPatch.PushBack(Dom::PatchOperation::RemoveOperation(mappedPath));
                        }

                        if (existingRowNode)
                        {
                            // value being replaced was a row; remove its node
                            auto& lastPathEntry = *(patchPath.end() - 1);
                            AZ_Assert(lastPathEntry.IsIndex(), "this removal entry must be an index!");
                            existingRowNode->m_parentNode->RemoveChildAtIndex(lastPathEntry.GetIndex());

                            // update former parent's match state, as its m_matchingDescendants may have changed
                            nodesToUpdate.insert(existingRowNode->m_parentNode);
                        }
                        else
                        {
                            // replaced node wasn't a row, so we need to re-cache the owning row's cached info
                            auto matchingRow = GetMatchNodeAtPath(rowPath);
                            nodesToUpdate.insert(matchingRow);
                        }

                        // if new value is a row, populate it but don't update the hierarchy yet
                        if (replacementIsRow)
                        {
                            PopulateNodesAtPath(patchPath, false);

                            auto* addedNode = GetMatchNodeAtPath(patchPath);
                            nodesToUpdate.insert(addedNode);
                            if (!existingRowNode)
                            {
                                // this value wasn't a row before, so its owning row needs to re-cache its info
                                nodesToUpdate.insert(addedNode->m_parentNode);
                            }
                        }
                        else
                        {
                            // replacement node isn't a row. Map the path and generate a patch operation
                            if (!mappedPath.IsEmpty())
                            {
                                outgoingPatch.PushBack(Dom::PatchOperation::AddOperation(mappedPath, sourceContents[patchPath]));
                            }

                            // non-row added so we need to re-cache the owning row's cached info
                            nodesToUpdate.insert(GetMatchNodeAtPath(rowPath));
                        }
                    }
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
                {
                    auto rowPath = GetRowPath(patchPath);
                    if (rowPath == patchPath)
                    {
                        // given path is a new row, populate our tree with match information for this node and any row descendants
                        // no need to generate a patch operation, that will be handled by GeneratePatch at the end
                        PopulateNodesAtPath(rowPath, false);

                        // defer updating to the end
                        auto* addedNode = GetMatchNodeAtPath(patchPath);
                        nodesToUpdate.insert(addedNode);
                    }
                    else
                    {
                        // added node wasn't a row. Map the path and generate a patch operation
                        auto mappedPath = MapFromSourcePath(patchPath);
                        if (!mappedPath.IsEmpty())
                        {
                            outgoingPatch.PushBack(Dom::PatchOperation::AddOperation(mappedPath, sourceContents[patchPath]));
                        }

                        // non-row added so we need to re-cache the owning row's cached info
                        nodesToUpdate.insert(GetMatchNodeAtPath(rowPath));
                    }
                }
                else
                {
                    AZ_Error("FilterAdapter", false, "patch operation not supported yet");
                }
            }
            for (auto node : nodesToUpdate)
            {
                CacheDomInfoForNode(sourceContents[GetSourcePathForNode(node)], node);
                UpdateMatchState(node);
            }

            // generate incremental patch that accounts for any rows that have been added, or whose filter state has changed
            GeneratePatch(Incremental, outgoingPatch);

            if (outgoingPatch.Size())
            {
                NotifyContentsChanged(outgoingPatch);
            }
        }
        else
        {
            // filter is inactive
            if (m_root)
            {
                // base model has changed, any cached MatchInfo is no longer valid
                delete m_root;
                m_root = nullptr;
            }

            // pass through existing patch
            NotifyContentsChanged(patch);
        }
    }

    RowFilterAdapter::MatchInfoNode* RowFilterAdapter::GetMatchNodeAtPath(const Dom::Path& sourcePath)
    {
        RowFilterAdapter::MatchInfoNode* currMatchState = m_root;
        if (sourcePath.Size() < 1)
        {
            // path is empty, return the root node
            return currMatchState;
        }

        for (const auto& pathEntry : sourcePath)
        {
            if (!pathEntry.IsIndex() || !currMatchState)
            {
                return nullptr;
            }
            const auto index = pathEntry.GetIndex();
            if (index >= currMatchState->m_childMatchState.size())
            {
                return nullptr;
            }
            currMatchState = currMatchState->m_childMatchState[index];
        }

        return currMatchState;
    }

    Dom::Path RowFilterAdapter::GetSourcePathForNode(const MatchInfoNode* matchNode)
    {
        Dom::Path sourcePath;
        auto addIndexToPath = [](const MatchInfoNode* currNode, Dom::Path& path, auto&& addIndexToPath) -> void
        {
            auto* parentNode = currNode->m_parentNode;
            if (parentNode)
            {
                addIndexToPath(parentNode, path, addIndexToPath);

                auto foundIter = AZStd::find_if(
                    parentNode->m_childMatchState.begin(),
                    parentNode->m_childMatchState.end(),
                    [currNode](auto testNode)
                    {
                        return (currNode == testNode);
                    });
                size_t index = AZStd::distance(parentNode->m_childMatchState.begin(), foundIter);
                path.Push(index);
            }
        };
        addIndexToPath(matchNode, sourcePath, addIndexToPath);
        return sourcePath;
    }

    void RowFilterAdapter::PopulateNodesAtPath(const Dom::Path& sourcePath, bool updateMatchState)
    {
        Dom::Path startingPath = GetRowPath(sourcePath);

        if (startingPath != sourcePath)
        {
            // path to add isn't a row, it's a column or an attribute. We're done!
            return;
        }
        else
        {
            // path is a node, populate its children (if any), dump its node info to the match string
            AZStd::function<void(MatchInfoNode*, const Dom::Value&)> recursivelyRegenerateMatches =
                [&](MatchInfoNode* matchState, const Dom::Value& value)
            {
                // precondition: value is a node of type row, matchState is blank, other than its m_parentNode
                CacheDomInfoForNode(value, matchState);

                const auto childCount = value.ArraySize();
                matchState->m_childMatchState.resize(childCount, nullptr);
                for (size_t arrayIndex = 0; arrayIndex < childCount; ++arrayIndex)
                {
                    auto& childValue = value[arrayIndex];
                    if (IsRow(childValue))
                    {
                        matchState->m_childMatchState[arrayIndex] = MakeNewNode(matchState, m_updateFrame);
                        auto& addedChild = matchState->m_childMatchState[arrayIndex];
                        recursivelyRegenerateMatches(addedChild, childValue);
                    }
                }
                // update ours and our ancestors' matching states
                if (updateMatchState)
                {
                    UpdateMatchState(matchState);
                }
            };

            // we should start with the parentPath, so peel off the new index from the end of the address
            auto& lastPathEntry = startingPath[startingPath.Size() - 1];
            auto parentPath = startingPath;
            parentPath.Pop();
            auto parentMatchState = GetMatchNodeAtPath(parentPath);

            auto newRowIndex = lastPathEntry.GetIndex();

            // inserting or appending child entry, add it to the correct position
            MatchInfoNode* addedChild = MakeNewNode(parentMatchState, m_updateFrame);
            parentMatchState->m_childMatchState.insert(parentMatchState->m_childMatchState.begin() + newRowIndex, addedChild);

            // recursively add descendants and cache their match status
            const auto& sourceContents = m_sourceAdapter->GetContents();
            recursivelyRegenerateMatches(addedChild, sourceContents[sourcePath]);
        }
    }

    void RowFilterAdapter::GenerateFullTree()
    {
        AZ_Assert(!m_root, "GenerateFullTree must only be called on an empty tree!");
        m_root = MakeNewNode(nullptr, m_updateFrame);
        const auto& sourceContents = m_sourceAdapter->GetContents();

        // we can assume all direct children of an adapter must be rows; populate each of them
        for (size_t topIndex = 0; topIndex < sourceContents.ArraySize(); ++topIndex)
        {
            PopulateNodesAtPath(Dom::Path({ Dom::PathEntry(topIndex) }), true);
        }
    }

    void RowFilterAdapter::CullUnmatchedChildRows(Dom::Value& rowValue, const MatchInfoNode* rowMatchNode)
    {
        auto& matchChildren = rowMatchNode->m_childMatchState;
        const bool sizesMatch = (rowValue.ArraySize() == matchChildren.size());
        AZ_Assert(sizesMatch, "DOM value and cached match node should have the same number of children!");

        if (sizesMatch)
        {
            auto valueIter = rowValue.MutableArrayBegin();
            for (auto matchIter = matchChildren.begin(); matchIter != matchChildren.end(); ++matchIter)
            {
                // non-row children will have a null MatchInfo -- we only need to worry about culling row values
                auto currMatch = *matchIter;
                if (currMatch != nullptr)
                {
                    if (currMatch->IncludeInFilter())
                    {
                        CullUnmatchedChildRows(*valueIter, *matchIter);
                    }
                    else
                    {
                        valueIter = rowValue.ArrayErase(valueIter);
                        continue; // we've already moved valueIter forward, skip the ++valueIter below
                    }
                }
                ++valueIter;
            }
        }
    }

    void RowFilterAdapter::GeneratePatch(PatchType patchType, Dom::Patch& patchToFill)
    {
        auto generatePatchOperations =
            [this, &patchToFill, patchType](
                MatchInfoNode* matchNode, Dom::Path mappedPath, const Dom::Value& sourceValue, auto&& generatePatchOperations) -> void
        {
            AZ_Assert(
                matchNode->m_childMatchState.size() == sourceValue.ArraySize(), "MatchInfoNode and sourceValue structure must match!");

            // This is the mapped (not source) child index. It will not increase when skipping a node or after removing a node
            int mappedChildIndex = 0;

            for (size_t sourceChildIndex = 0, sourceChildCount = matchNode->m_childMatchState.size(); sourceChildIndex < sourceChildCount;
                 ++sourceChildIndex)
            {
                auto currChild = matchNode->m_childMatchState[sourceChildIndex];
                if (currChild && (currChild->m_lastFilterUpdateFrame == m_updateFrame || patchType != Incremental))
                {
                    if (currChild->IncludeInFilter())
                    {
                        if (patchType == Incremental)
                        {
                            // child was added this frame and should be included, generate an add operation
                            Dom::Value culledValue = sourceValue[sourceChildIndex];
                            CullUnmatchedChildRows(culledValue, currChild);
                            patchToFill.PushBack(Dom::PatchOperation::AddOperation(mappedPath / mappedChildIndex, culledValue));
                            ++mappedChildIndex;
                        }
                        else
                        {
                            // RemovalsFromSource and PatchToSource types don't generate patches for matching children, but have to recurse
                            // into them to check deeper
                            generatePatchOperations(
                                currChild, mappedPath / mappedChildIndex, sourceValue[sourceChildIndex], generatePatchOperations);
                            ++mappedChildIndex;
                        }
                    }
                    else
                    {
                        // updated node is not IncludeInFilter
                        if (patchType == PatchToSource)
                        {
                            // if we're trying to generate a patch to get us back to the source model, add this missing value back to the
                            // DOM
                            patchToFill.PushBack(
                                Dom::PatchOperation::AddOperation(mappedPath / mappedChildIndex, sourceValue[sourceChildIndex]));
                            ++mappedChildIndex;
                        }
                        else
                        {
                            // otherwise, this is an non-matching node that should be removed. Generate removal and don't increment
                            // mappedChildIndex
                            patchToFill.PushBack(Dom::PatchOperation::RemoveOperation(mappedPath / mappedChildIndex));
                        }
                    }
                }
                else // patchType is Incremental and currChild wasn't updated this frame
                {
                    // entries will be null for non-row children; recurse only into included row children
                    if (currChild)
                    {
                        if (currChild->IncludeInFilter())
                        {
                            generatePatchOperations(
                                currChild, mappedPath / mappedChildIndex, sourceValue[sourceChildIndex], generatePatchOperations);
                            ++mappedChildIndex;
                        }
                    }
                    else
                    {
                        // null (non-row) child, include but skip
                        ++mappedChildIndex;
                    }
                }
            }
        };

        // generate Dom::PatchOperations from the root down and add them to patch
        generatePatchOperations(m_root, Dom::Path(), m_sourceAdapter->GetContents(), generatePatchOperations);
    }

    void RowFilterAdapter::UpdateMatchState(MatchInfoNode* rowState)
    {
        bool usedToMatch = rowState->IncludeInFilter();
        bool matchedDirectly = rowState->m_matchesSelf;
        rowState->m_matchesSelf = MatchesFilter(rowState); // update own match state

        // if the filter is set to treat descendants of a match as matching themselves,
        // and we now directly match, update all descendants
        if (m_includeAllMatchDescendants && rowState->m_matchesSelf != matchedDirectly)
        {
            UpdateMatchDescendants(rowState);
        }

        auto& matchingChildren = rowState->m_matchingDescendants;
        for (auto childIter = matchingChildren.begin(); childIter != matchingChildren.end(); /* omitted */)
        {
            // it's a child's job to update its parents, but a parent still has to check if any
            // formerly matching children have been removed
            auto& currChildren = rowState->m_childMatchState;
            if (AZStd::find(currChildren.begin(), currChildren.end(), *childIter) == currChildren.end())
            {
                childIter = matchingChildren.erase(childIter);
            }
            else
            {
                ++childIter;
            }
        }

        bool hasMatchingAncestor = false;
        if (m_includeAllMatchDescendants)
        {
            MatchInfoNode* ancestor = rowState->m_parentNode;
            while (ancestor && !hasMatchingAncestor)
            {
                if (ancestor->m_matchesSelf)
                {
                    hasMatchingAncestor = true;
                }
                ancestor = ancestor->m_parentNode;
            }
        }
        rowState->m_hasMatchingAncestor = hasMatchingAncestor;

        MatchInfoNode* currState = rowState;
        bool nowMatches = currState->IncludeInFilter();

        // if this node changed, update each ancestor until updating their m_matchingDescendants doesn't change their inclusion state
        while (nowMatches != usedToMatch && currState->m_parentNode)
        {
            // this node's match state has changed, note that this change happened this frame
            currState->m_lastFilterUpdateFrame = m_updateFrame;

            // now note the parent's state before updating its m_matchingDescendants,
            // so we can later check if changing its m_matchingDescendants reversed its match state
            usedToMatch = currState->m_parentNode->IncludeInFilter();
            if (nowMatches)
            {
                currState->m_parentNode->m_matchingDescendants.insert(currState);
            }
            else
            {
                currState->m_parentNode->m_matchingDescendants.erase(currState);
            }
            nowMatches = currState->m_parentNode->IncludeInFilter();
            currState = currState->m_parentNode;
        }

        // handle the case where the final parentless node changed its match state
        // NB: this could also be the *first* node that changed, if it had no parent
        if (usedToMatch != nowMatches)
        {
            currState->m_lastFilterUpdateFrame = m_updateFrame;
        }
    }

    void RowFilterAdapter::UpdateMatchDescendants(MatchInfoNode* startNode)
    {
        for (auto* childNode : startNode->m_childMatchState)
        {
            if (childNode)
            {
                UpdateMatchState(childNode);
                UpdateMatchDescendants(childNode);
            }
        }
    }
} // namespace AZ::DocumentPropertyEditor
