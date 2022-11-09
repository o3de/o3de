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
                GeneratePatch(RemovalsFromSource);
            }
            else
            {
                // generate a patch that adds in any filtered out nodes, but leave tree alone in case the filter gets reactivated
                GeneratePatch(PatchToSource);
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
            GeneratePatch(Incremental);
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

        if (m_filterActive)
        {
            const auto& sourceContents = m_sourceAdapter->GetContents();
            for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
            {
                const auto& patchPath = operationIterator->GetDestinationPath();
                if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
                {
                    auto matchingRow = GetMatchNodeAtPath(patchPath);
                    if (matchingRow)
                    {
                        // node being removed is a row. We need to remove it from our graph
                        auto& parentContainer = matchingRow->m_parentNode->m_childMatchState;
                        auto& lastPathEntry = *(patchPath.end() - 1);
                        AZ_Assert(lastPathEntry.IsIndex(), "this removal entry must be an index!");
                        parentContainer.erase(parentContainer.begin() + lastPathEntry.GetIndex());

                        // update former parent's match state, as its m_matchingDescendants may have changed
                        UpdateMatchState(matchingRow->m_parentNode);
                    }
                    else
                    {
                        // removed node wasn't a row, so we need to re-cache the owning row's cached info
                        auto parentPath = patchPath;
                        parentPath.Pop();
                        auto parentRow = GetMatchNodeAtPath(parentPath);
                        CacheDomInfoForNode(sourceContents[parentPath], parentRow);
                        UpdateMatchState(parentRow);
                    }
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
                {
                    auto rowPath = GetRowPath(patchPath);

                    // replace operations can change child values from or into rows. Handle all cases
                    auto existingMatchInfo = GetMatchNodeAtPath(patchPath);
                    if (patchPath == rowPath)
                    {
                        // replacement value is a row, populate it
                        PopulateNodesAtPath(patchPath, true);
                        if (!existingMatchInfo)
                        {
                            // this value wasn't a row before, so its parent needs to re-cache its info
                            auto parentNode = GetMatchNodeAtPath(patchPath)->m_parentNode;
                            auto parentPath = patchPath;
                            parentPath.Pop();
                            CacheDomInfoForNode(sourceContents[parentPath], parentNode);
                            UpdateMatchState(parentNode);
                        }
                    }
                    else
                    {
                        // not a row now. Check if it was before
                        if (existingMatchInfo)
                        {
                            // node being replaced was a row. We need to remove it from our graph
                            auto& parentContainer = existingMatchInfo->m_parentNode->m_childMatchState;
                            parentContainer.erase(parentContainer.begin() + (patchPath.end() - 1)->GetIndex());

                            // update former parent's match state, as its m_matchingDescendants may have changed
                            UpdateMatchState(existingMatchInfo->m_parentNode);
                        }
                        else
                        {
                            // replaced node wasn't a row, so we need to re-cache the owning row's cached info
                            auto matchingRow = GetMatchNodeAtPath(rowPath);
                            CacheDomInfoForNode(sourceContents[rowPath], matchingRow);
                            UpdateMatchState(matchingRow);
                        }
                    }
                }
                else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
                {
                    auto rowPath = GetRowPath(patchPath);
                    if (rowPath == patchPath)
                    {
                        // given path is a new row, populate our tree with match information for this node and any row descendants
                        PopulateNodesAtPath(rowPath, false);
                    }
                    else
                    {
                        // added node wasn't a row, so we need to re-cache the owning row's cached info
                        auto matchingRow = GetMatchNodeAtPath(rowPath);
                        CacheDomInfoForNode(sourceContents[rowPath], matchingRow);
                        UpdateMatchState(matchingRow);
                    }
                }
                else
                {
                    AZ_Error("FilterAdapter", false, "patch operation not supported yet");
                }
            }
            GeneratePatch(Incremental);
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

    void RowFilterAdapter::PopulateNodesAtPath(const Dom::Path& sourcePath, bool replaceExisting)
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
                UpdateMatchState(matchState);
            };

            // we should start with the parentPath, so peel off the new index from the end of the address
            auto& lastPathEntry = startingPath[startingPath.Size() - 1];
            auto parentPath = startingPath;
            parentPath.Pop();
            auto parentMatchState = GetMatchNodeAtPath(parentPath);

            MatchInfoNode* addedChild = nullptr;
            if (replaceExisting)
            {
                // destroy existing entry, if present
                AZ_Assert(lastPathEntry.IsIndex(), "if we're replacing, the last entry in the the path must by a valid index");
                auto newRowIndex = lastPathEntry.GetIndex();
                const bool hasEntryToReplace = (parentMatchState->m_childMatchState.size() > newRowIndex);
                AZ_Assert(hasEntryToReplace, "PopulateNodesAtPath was called with replaceExisting, but no existing entry exists!");
                if (hasEntryToReplace)
                {
                    delete parentMatchState->m_childMatchState[newRowIndex];
                    addedChild = MakeNewNode(parentMatchState, m_updateFrame);
                    parentMatchState->m_childMatchState[newRowIndex] = addedChild;
                }
                else
                {
                    // this shouldn't happen!
                    return;
                }
            }
            else
            {
                auto newRowIndex = lastPathEntry.GetIndex();

                // inserting or appending child entry, add it to the correct position
                addedChild = MakeNewNode(parentMatchState, m_updateFrame);
                parentMatchState->m_childMatchState.insert(parentMatchState->m_childMatchState.begin() + newRowIndex, addedChild);
            }

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
            PopulateNodesAtPath(Dom::Path({ Dom::PathEntry(topIndex) }), false);
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

    void RowFilterAdapter::GeneratePatch(PatchType patchType)
    {
        Dom::Patch patch;

        auto generatePatchOperations =
            [this, &patch, patchType](
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
                            patch.PushBack(Dom::PatchOperation::AddOperation(mappedPath / mappedChildIndex, culledValue));
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
                            patch.PushBack(Dom::PatchOperation::AddOperation(mappedPath / mappedChildIndex, sourceValue[sourceChildIndex]));
                            ++mappedChildIndex;
                        }
                        else
                        {
                            // otherwise, this is an non-matching node that should be removed. Generate removal and don't increment
                            // mappedChildIndex
                            patch.PushBack(Dom::PatchOperation::RemoveOperation(mappedPath / mappedChildIndex));
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
        if (patch.Size())
        {
            NotifyContentsChanged(patch);
        }
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
