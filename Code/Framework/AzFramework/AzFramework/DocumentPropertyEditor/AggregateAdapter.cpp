/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AggregateAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    RowAggregateAdapter::RowAggregateAdapter()
        : m_rootNode(AZStd::make_unique<AggregateNode>())
    {
    }

    RowAggregateAdapter::~RowAggregateAdapter()
    {
    }

    void RowAggregateAdapter::AddAdapter(DocumentAdapterPtr sourceAdapter)
    {
        // capture the actual adapter, not just the index, as adding or removing adapters could change the index
        auto& newAdapterInfo = m_adapters.emplace_back(AZStd::make_unique<AdapterInfo>());
        newAdapterInfo->adapter = sourceAdapter;

        newAdapterInfo->resetHandler = DocumentAdapter::ResetEvent::Handler(
            [this, sourceAdapter]()
            {
                this->HandleAdapterReset(sourceAdapter);
            });
        sourceAdapter->ConnectResetHandler(newAdapterInfo->resetHandler);

        newAdapterInfo->changedHandler = DocumentAdapter::ChangedEvent::Handler(
            [this, sourceAdapter](const Dom::Patch& patch)
            {
                this->HandleDomChange(sourceAdapter, patch);
            });
        sourceAdapter->ConnectChangedHandler(newAdapterInfo->changedHandler);

        newAdapterInfo->domMessageHandler = AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler(
            [this, sourceAdapter](const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value)
            {
                this->HandleDomMessage(sourceAdapter, message, value);
            });
        sourceAdapter->ConnectMessageHandler(newAdapterInfo->domMessageHandler);

        const auto adapterIndex = m_adapters.size() - 1;
        PopulateNodesForAdapter(adapterIndex);
        NotifyResetDocument();
    }

    void RowAggregateAdapter::RemoveAdapter(DocumentAdapterPtr sourceAdapter)
    {
        // TODO: https://github.com/o3de/o3de/issues/15609
        (void)sourceAdapter;
    }

    void RowAggregateAdapter::ClearAdapters()
    {
        m_adapters.clear();
        m_rootNode = AZStd::make_unique<AggregateNode>();
        NotifyResetDocument();
    }

    bool RowAggregateAdapter::AggregateNode::HasEntryForAdapter(size_t adapterIndex)
    {
        return (m_pathEntries.size() > adapterIndex && m_pathEntries[adapterIndex] != InvalidEntry);
    }

    Dom::Path RowAggregateAdapter::AggregateNode::GetPathForAdapter(size_t adapterIndex)
    {
        Dom::Path pathForAdapter;
        auto addParentThenSelf = [&pathForAdapter, adapterIndex](AggregateNode* currentNode, auto&& addParentThenSelf) -> bool
        {
            bool succeeded = true;
            if (currentNode->m_parent)
            {
                succeeded = addParentThenSelf(currentNode->m_parent, addParentThenSelf);
            }
            if (succeeded && adapterIndex < currentNode->m_pathEntries.size())
            {
                const size_t pathEntry = currentNode->m_pathEntries[adapterIndex];
                succeeded = (pathEntry != InvalidEntry);
                if (succeeded)
                {
                    pathForAdapter.Push(pathEntry);
                }
            }
            return succeeded;
        };
        auto validPath = addParentThenSelf(this, addParentThenSelf);
        return (validPath ? pathForAdapter : Dom::Path());
    }

    void RowAggregateAdapter::AggregateNode::AddEntry(size_t adapterIndex, size_t pathEntryIndex, bool matchesOtherEntries)
    {
        const auto currentSize = m_pathEntries.size();
        if (currentSize <= adapterIndex)
        {
            m_pathEntries.resize(adapterIndex + 1, InvalidEntry);
        }
        m_pathEntries[adapterIndex] = pathEntryIndex;

        // set m_allEntriesMatch so that the Adapter knows whether to show a "values differ" type of node
        m_allEntriesMatch = matchesOtherEntries;

        AZ_Assert(m_parent, "Attempt to add entry to orphaned node!");
        if (m_parent)
        {
            // add mapping to parent so that we can easily walk an adapter's path to get to the desired AggregateNode
            const auto parentSize = m_parent->m_pathIndexToChildMaps.size();
            if (parentSize <= adapterIndex)
            {
                m_parent->m_pathIndexToChildMaps.resize(adapterIndex + 1);
            }
            else
            {
                // shift any subsequent entries by 1 to make room for this new entry, then add it
                m_parent->ShiftChildIndices(adapterIndex, pathEntryIndex, 1);
            }
            m_parent->m_pathIndexToChildMaps[adapterIndex][pathEntryIndex] = this;
        }
    }

    bool RowAggregateAdapter::AggregateNode::RemoveEntry(size_t adapterIndex)
    {
        // find entry in parent's index map and remove it
        auto& parentsMap = m_parent->m_pathIndexToChildMaps[adapterIndex];
        auto mapEntryIter = AZStd::find_if(
            parentsMap.begin(),
            parentsMap.end(),
            [this](auto& mapEntry)
            {
                return (mapEntry.second == this);
            });
        AZ_Assert(mapEntryIter != parentsMap.end(), "child must be present in parent's map!");
        // remove entry from map and shift any later siblings' indices down by one
        parentsMap.erase(mapEntryIter);
        m_parent->ShiftChildIndices(adapterIndex, mapEntryIter->first, -1);

        bool nodeWasDestroyed = false;
        m_pathEntries[adapterIndex] = InvalidEntry;

        auto remainingEntries = m_pathEntries.size();
        while (remainingEntries && m_pathEntries[remainingEntries - 1] == InvalidEntry)
        {
            --remainingEntries;
        }

        if (!remainingEntries)
        {
            // no remaining entries, destroy this node
            auto* parentNode = m_parent;
            auto& parentsChildList = parentNode->m_childRows;
            auto thisNodeIter = AZStd::find_if(
                parentsChildList.begin(),
                parentsChildList.end(),
                [this](auto& mapEntry)
                {
                    return (mapEntry.get() == this);
                });
            AZ_Assert(thisNodeIter != parentsChildList.end(), "child must be present in parent's list!");

            // entry is a unique_ptr, erasing it will destroy this node
            parentsChildList.erase(thisNodeIter);
            nodeWasDestroyed = true;
        }
        else if (remainingEntries < m_pathEntries.size())
        {
            // some valid entries remain, resize appropriately
            m_pathEntries.resize(remainingEntries);
        }
        return nodeWasDestroyed;
    }

    size_t RowAggregateAdapter::AggregateNode::EntryCount() const
    {
        // count all entries that are not InvalidEntry
        return AZStd::count_if(
            m_pathEntries.begin(),
            m_pathEntries.end(),
            [&](size_t currentEntry)
            {
                return (currentEntry != InvalidEntry);
            });
    }

    void RowAggregateAdapter::AggregateNode::ShiftChildIndices(size_t adapterIndex, size_t startIndex, int delta)
    {
        AZ_Assert(adapterIndex < m_pathIndexToChildMaps.size(), "m_pathIndexToChildMaps is not correctly sized!");
        auto& adapterChildMap = m_pathIndexToChildMaps[adapterIndex];

        auto firstEntry = adapterChildMap.lower_bound(startIndex);
        if (firstEntry != adapterChildMap.end())
        {
            auto applyDelta = [adapterIndex, delta, &adapterChildMap](auto& iterator)
            {
                // make an iterator that points to the next element for the re-insertion hint
                auto reinsertPos = iterator;
                ++reinsertPos;

                auto entry = adapterChildMap.extract(iterator);
                entry.mapped()->m_pathEntries[adapterIndex] += delta;
                entry.key() += delta;
                iterator = adapterChildMap.insert(reinsertPos, AZStd::move(entry));
            };

            const bool incrementing = (delta > 0);
            if (incrementing)
            {
                /* we don't want the map entries to pass each other and require the internal map to change structure,
                   so start at the end and move backwards if we're incrementing the indices */
                auto mapIter = adapterChildMap.end();
                do
                {
                    // we want to applyDelta up to and including the firstEntry
                    --mapIter;
                    applyDelta(mapIter);
                } while (mapIter != firstEntry);
            }
            else
            {
                // not incrementing, so we're removing from the front this time
                for (auto mapIter = firstEntry; mapIter != adapterChildMap.end(); ++mapIter)
                {
                    applyDelta(mapIter);
                }
            }
        }
    }

    void RowAggregateAdapter::HandleAdapterReset(DocumentAdapterPtr adapter)
    {
        // TODO https://github.com/o3de/o3de/issues/15610
        (void)adapter;
    }

    void RowAggregateAdapter::HandleDomChange(DocumentAdapterPtr adapter, const Dom::Patch& patch)
    {
        const auto adapterIndex = GetIndexForAdapter(adapter);

        for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
        {
            const auto& patchPath = operationIterator->GetDestinationPath();
            if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
            {
                auto* nodeAtPath = GetNodeAtAdapterPath(adapterIndex, patchPath, PathMatch::ExactMatch);
                if (nodeAtPath)
                {
                    // this removal was a row, remove its node entry
                    ProcessRemoval(nodeAtPath, adapterIndex);
                }
                else
                {
                    // this removal was not a full row, but it still shifts subsequent entries, so account for that in the parent
                    auto parentPath = patchPath;
                    parentPath.Pop();
                    auto* parentNode = GetNodeAtAdapterPath(adapterIndex, parentPath, PathMatch::ExactMatch);
                    if (parentNode)
                    {
                        parentNode->ShiftChildIndices(adapterIndex, patchPath.Back().GetIndex(), -1);
                    }
                    // removing a column or attribute entry might affect the row it belongs to. Update it
                    auto* rowNode = GetNodeAtAdapterPath(adapterIndex, patchPath, PathMatch::NearestRow);
                    UpdateAndPatchNode(rowNode, adapterIndex);
                }
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
            {
                // nodes represent entire aggregate rows, so if there's a node, it a replace for a full row
                auto* nodeAtPath = GetNodeAtAdapterPath(adapterIndex, patchPath, PathMatch::ExactMatch);
                const bool replacementIsRow = IsRow(operationIterator->GetValue());
                if (!nodeAtPath)
                {
                    // there isn't currently a row at that path. Find and update the first row node in this patch's ancestry
                    AggregateNode* rowNode = GetNodeAtAdapterPath(adapterIndex, patchPath, PathMatch::NearestRow);
                    AZ_Assert(rowNode, "all paths should have a nearest row!");
                    UpdateAndPatchNode(rowNode, adapterIndex);

                    // if the replacement is a row, we need to add it to the hierarchy
                    if (replacementIsRow)
                    {
                        // handle column being replaced by row. Add the new row child
                        AddChildRow(adapterIndex, rowNode, operationIterator->GetValue(), patchPath.Back().GetIndex(), true);
                    }
                }
                else
                {
                    // handle row being replaced
                    if (replacementIsRow)
                    {
                        auto parentNode = nodeAtPath->m_parent; // cache of parent, since nodeAtPath may be imminently removed
                        ProcessRemoval(nodeAtPath, adapterIndex);
                        AddChildRow(adapterIndex, parentNode, operationIterator->GetValue(), patchPath.Back().GetIndex(), true);
                    }
                    else
                    {
                        // replacing a row with a non-row. Update the parent and remove the existing row
                        UpdateAndPatchNode(nodeAtPath->m_parent, adapterIndex);
                        ProcessRemoval(nodeAtPath, adapterIndex);
                    }
                }
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                auto destinationPath = patchPath;
                auto lastPathEntry = destinationPath.Back();
                destinationPath.Pop();
                auto parentNode = GetNodeAtAdapterPath(adapterIndex, destinationPath, PathMatch::ExactMatch);

                if (parentNode && IsRow(operationIterator->GetValue()))
                {
                    // adding a full row to the root or another row
                    AZ_Assert(lastPathEntry.IsIndex(), "new addition is a row, it must be addressed by index!");
                    const auto childIndex = lastPathEntry.GetIndex();
                    AddChildRow(adapterIndex, parentNode, operationIterator->GetValue(), childIndex, true);
                }
                else
                {
                    // either a column or an attribute was added, update the nearest row as necessary
                    if (parentNode)
                    {
                        // this add was not a row, but it still shifts subsequent entries, so account for that in the parent
                        parentNode->ShiftChildIndices(adapterIndex, destinationPath.Back().GetIndex(), 1);
                    }

                    AggregateNode* rowNode = GetNodeAtAdapterPath(adapterIndex, patchPath, PathMatch::NearestRow);
                    AZ_Assert(rowNode, "all paths should have a nearest row!");
                    UpdateAndPatchNode(rowNode, adapterIndex);
                }
            }
            else
            {
                // Note that many some patch operations are still unsupported at this time
                // Please see: https://github.com/o3de/o3de/issues/15612
                AZ_Assert(0, "RowAggregateAdapter: patch operation type not supported yet!");
            }
        }
    }

    void RowAggregateAdapter::HandleDomMessage(
        [[maybe_unused]] DocumentAdapterPtr adapter,
        [[maybe_unused]] const AZ::DocumentPropertyEditor::AdapterMessage& message,
        [[maybe_unused]] Dom::Value& value)
    {
        // TODO forwarding all of these isn't desirable, test to see if we need to conditionally forward this
        // https://github.com/o3de/o3de/issues/15611
    }

    void RowAggregateAdapter::EvaluateAllEntriesMatch(AggregateNode* node)
    {
        bool allEntriesMatch = true;

        if (node->EntryCount() > 1)
        {
            auto comparisonRow = m_adapters[0]->adapter->GetContents()[node->GetPathForAdapter(0)];
            for (size_t index = 1, numEntries = node->m_pathEntries.size(); index < numEntries && allEntriesMatch; ++index)
            {
                if (node->m_pathEntries[index] != AggregateNode::InvalidEntry)
                {
                    auto currAdapterPath = node->GetPathForAdapter(index);
                    AZ_Assert(!currAdapterPath.IsEmpty(), "ancestorRowNode had an entry for this adapter -- it must have a valid path!");
                    auto currRowValue = m_adapters[index]->adapter->GetContents()[currAdapterPath];
                    allEntriesMatch = ValuesMatch(comparisonRow, currRowValue);
                }
            }
        }
        node->m_allEntriesMatch = allEntriesMatch;
    }

    size_t RowAggregateAdapter::GetIndexForAdapter(const DocumentAdapterPtr& adapter)
    {
        for (size_t adapterIndex = 0, numAdapters = m_adapters.size(); adapterIndex < numAdapters; ++adapterIndex)
        {
            if (m_adapters[adapterIndex]->adapter == adapter)
                return adapterIndex;
        }
        AZ_Warning("RowAggregateAdapter", false, "GetIndexForAdapter called with unknown adapter!");
        return size_t(-1);
    }

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::GetNodeAtAdapterPath(
        size_t adapterIndex, const Dom::Path& path, PathMatch pathMatch)
    {
        AggregateNode* currNode = m_rootNode.get();
        if (path.Size() < 1)
        {
            // path is empty, return the root node
            return currNode;
        }

        for (const auto& pathEntry : path)
        {
            if (!pathEntry.IsIndex() || adapterIndex >= currNode->m_pathIndexToChildMaps.size())
            {
                // this path includes a non-index entry or an index out of bounds -- we can search no deeper
                return (pathMatch == PathMatch::ExactMatch ? nullptr : currNode);
            }
            auto& pathMap = currNode->m_pathIndexToChildMaps[adapterIndex];
            const auto index = pathEntry.GetIndex();
            if (auto foundIter = pathMap.find(index); foundIter != pathMap.end())
            {
                currNode = foundIter->second;
            }
            else
            {
                // nothing for that path entry. If pathMatch is NearestRow, assume this is a column entry,
                // and return the most recent row node
                return (pathMatch == PathMatch::ExactMatch ? nullptr : currNode);
            }
        }
        return currNode;
    }

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::GetNodeAtPath(const Dom::Path& aggregatePath)
    {
        AggregateNode* currNode = m_rootNode.get();
        if (aggregatePath.Size() < 1)
        {
            // path is empty, return the root node
            return currNode;
        }

        auto getCompleteChildAtIndex = [&](AggregateNode* node, size_t childIndex) -> AggregateNode*
        {
            size_t numCompleteRows = 0;
            size_t testIndex = 0;
            while (numCompleteRows <= childIndex && testIndex < node->m_childRows.size())
            {
                auto& currChild = node->m_childRows[testIndex];
                if (NodeIsComplete(currChild.get()))
                {
                    ++numCompleteRows;
                }
                if (numCompleteRows > childIndex)
                {
                    return node->m_childRows[testIndex].get();
                }
                ++testIndex;
            }
            return nullptr;
        };

        for (const auto& pathEntry : aggregatePath)
        {
            if (!pathEntry.IsIndex())
            {
                // this path includes a non-index entry, and is therefore not a row
                return nullptr;
            }
            const auto index = pathEntry.GetIndex();
            currNode = getCompleteChildAtIndex(currNode, index);
            if (!currNode)
            {
                return nullptr;
            }
        }
        return currNode;
    }

    Dom::Path RowAggregateAdapter::GetPathForNode(AggregateNode* node)
    {
        // verify that this and all ancestors have entries for each adapter,
        // otherwise there is no path for this node, as it won't be included in the contents
        auto* currNode = node;
        while (currNode)
        {
            if (currNode != m_rootNode.get() && !NodeIsComplete(currNode))
            {
                return Dom::Path();
            }
            currNode = currNode->m_parent;
        }
        Dom::Path nodePath;
        auto addParentThenSelf = [this, &nodePath](AggregateNode* currentNode, auto&& addParentThenSelf) -> void
        {
            auto* currParent = currentNode->m_parent;
            if (currParent)
            {
                addParentThenSelf(currParent, addParentThenSelf);

                size_t currIndex = 0;
                for (auto& currChild : currParent->m_childRows)
                {
                    if (currChild.get() == currentNode)
                    {
                        nodePath.Push(currIndex);
                        break;
                    }
                    else if (NodeIsComplete(currChild.get()))
                    {
                        ++currIndex;
                    }
                }
            }
        };
        addParentThenSelf(node, addParentThenSelf);
        return nodePath;
    }

    Dom::Value RowAggregateAdapter::GetComparisonRow(AggregateNode* aggregateNode, size_t omitAdapterIndex)
    {
        for (size_t adapterIndex = 0, numAdapters = m_adapters.size(); adapterIndex < numAdapters; ++adapterIndex)
        {
            if (adapterIndex != omitAdapterIndex) // don't generate a value from the adapter at omitAdapterIndex, if it exists
            {
                Dom::Path pathToComparisonValue = aggregateNode->GetPathForAdapter(adapterIndex);
                if (!pathToComparisonValue.IsEmpty())
                {
                    auto comparisonRow = m_adapters[adapterIndex]->adapter->GetContents()[pathToComparisonValue];
                    RemoveChildRows(comparisonRow);
                    return comparisonRow;
                }
            }
        }
        // if we reached here, there is no valid adapter that isn't at omitAdapterIndex with a value for this node
        return Dom::Value();
    }

    Dom::Value RowAggregateAdapter::GetValueForNode(AggregateNode* aggregateNode)
    {
        return (
            aggregateNode->m_allEntriesMatch || !m_generateDiffRows ? GenerateAggregateRow(aggregateNode)
                                                                    : GenerateValuesDifferRow(aggregateNode));
    }

    Dom::Value RowAggregateAdapter::GetValueHierarchyForNode(AggregateNode* aggregateNode)
    {
        Dom::Value returnValue;

        // only create a value if this node is represented by all member adapters
        if (aggregateNode && NodeIsComplete(aggregateNode))
        {
            // create a row value for this node
            returnValue = Dom::Value::CreateNode(Nodes::Row::Name);

            /* add all row children first (before any labels or PropertyHandlers in this row),
                so that functions like GetPathForNode can make simplifying assumptions */
            for (auto& currChild : aggregateNode->m_childRows)
            {
                Dom::Value childValue = GetValueHierarchyForNode(currChild.get());

                if (!childValue.IsNull())
                {
                    returnValue.ArrayPushBack(childValue);
                }
            }
            // row children have been added, now add the actual label/PropertyEditor children
            auto childlessRow = GetValueForNode(aggregateNode);
            if (!childlessRow.IsArrayEmpty())
            {
                returnValue.ArrayReserve(returnValue.ArraySize() + childlessRow.ArraySize());
                AZStd::move(
                    childlessRow.MutableArrayBegin(), childlessRow.MutableArrayEnd(), AZStd::back_inserter(returnValue.GetMutableArray()));
            }
        }
        return returnValue;
    }

    void RowAggregateAdapter::PopulateNodesForAdapter(size_t adapterIndex)
    {
        auto sourceAdapter = m_adapters[adapterIndex]->adapter;
        const auto& sourceContents = sourceAdapter->GetContents();
        PopulateChildren(adapterIndex, sourceContents, m_rootNode.get());
    }

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::AddChildRow(
        size_t adapterIndex, AggregateNode* parentNode, const Dom::Value& childValue, size_t childIndex, bool generateAddPatch)
    {
        AggregateNode* addedToNode = nullptr;

        // check each existing child to see if we belong there.
        for (auto matchIter = parentNode->m_childRows.begin(), endIter = parentNode->m_childRows.end();
             !addedToNode && matchIter != endIter;
             ++matchIter)
        {
            AggregateNode* possibleMatch = matchIter->get();

            // make sure there isn't already an entry for this adapter. This can happen in
            // edge cases where multiple rows can match, like in multi-sets
            if (!possibleMatch->HasEntryForAdapter(adapterIndex))
            {
                auto comparisonRow = GetComparisonRow(possibleMatch);
                if (SameRow(childValue, comparisonRow))
                {
                    const bool allEntriesMatch = possibleMatch->m_allEntriesMatch && ValuesMatch(childValue, comparisonRow);
                    possibleMatch->AddEntry(adapterIndex, childIndex, allEntriesMatch);
                    PopulateChildren(adapterIndex, childValue, possibleMatch);
                    addedToNode = possibleMatch;
                }
            }
        }
        if (!addedToNode)
        {
            // didn't find an existing child to own us, add a new one and attach to it
            auto& newNode = parentNode->m_childRows.emplace_back(AZStd::make_unique<AggregateNode>());
            newNode->m_parent = parentNode;
            newNode->AddEntry(adapterIndex, childIndex, true);
            PopulateChildren(adapterIndex, childValue, newNode.get());
            addedToNode = newNode.get();
        }

        if (addedToNode && generateAddPatch && NodeIsComplete(addedToNode))
        {
            // the aggregate node that this add affected is now complete
            NotifyContentsChanged(
                { Dom::PatchOperation::AddOperation(GetPathForNode(addedToNode), GetValueHierarchyForNode(addedToNode)) });
        }

        return addedToNode;
    }

    void RowAggregateAdapter::PopulateChildren(size_t adapterIndex, const Dom::Value& parentValue, AggregateNode* parentNode)
    {
        // go through each DOM child of parentValue, ignoring non-rows
        const auto numChildren = parentValue.ArraySize();
        for (size_t childIndex = 0; childIndex < numChildren; ++childIndex)
        {
            const auto& childValue = parentValue[childIndex];

            // the RowAggregateAdapter groups nodes by row, so we ignore non-child rows here
            if (IsRow(childValue))
            {
                AddChildRow(adapterIndex, parentNode, childValue, childIndex, false);
            }
        }
    }

    void RowAggregateAdapter::ProcessRemoval(AggregateNode* rowNode, size_t adapterIndex)
    {
        if (NodeIsComplete(rowNode))
        {
            // value has changed for this node, and it no longer matches its peers.
            // If this node was complete before, it isn't now
            auto nodePath = GetPathForNode(rowNode);
            if (!nodePath.IsEmpty())
            {
                NotifyContentsChanged({ Dom::PatchOperation::RemoveOperation(nodePath) });
            }
        }
        if (!rowNode->RemoveEntry(adapterIndex))
        {
            // entry was removed, but node still has entries. Update their "all entries match" status
            EvaluateAllEntriesMatch(rowNode);
        }
    }

    void RowAggregateAdapter::UpdateAndPatchNode(AggregateNode* rowNode, size_t adapterIndex)
    {
        auto& adapter = m_adapters[adapterIndex]->adapter;
        auto rowPath = rowNode->GetPathForAdapter(adapterIndex);
        const bool nodeWasComplete = NodeIsComplete(rowNode);
        if (rowNode->EntryCount() > 1)
        {
            // if there's a node for this value, and it's not the only entry in the node,
            // we need to check if it still belongs with the other entries
            auto comparisonRow = GetComparisonRow(rowNode, adapterIndex);
            AZ_Assert(!comparisonRow.IsNull(), "there should always be a valid comparison value for a node with more than one entry");

            // this patch operation is already reflected in the adapter's DOM, retrieve its new value
            auto adapterRow = adapter->GetContents()[rowPath];
            if (SameRow(comparisonRow, adapterRow))
            {
                EvaluateAllEntriesMatch(rowNode);

                if (nodeWasComplete)
                {
                    // value has changed for this node, but its still complete because it's "SameRow"
                    // Issue a replacement patch
                    auto nodePath = GetPathForNode(rowNode);
                    if (!nodePath.IsEmpty())
                    {
                        NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(nodePath, GetValueHierarchyForNode(rowNode)) });
                    }
                }
            }
            else
            {
                // no longer the same row, remove it from the parent and re-add with the new value
                // cache off the parent node, since RemoveEntry may well delete ancestorRowNode
                auto* parentNode = rowNode->m_parent;
                ProcessRemoval(rowNode, adapterIndex);
                AddChildRow(adapterIndex, parentNode, adapterRow, rowPath.Back().GetIndex(), true);
            }
        }
        else
        {
            // handle case where there's only one entry in this node, but it might've changed.
            auto* parentNode = rowNode->m_parent;
            const bool hasSiblings = (parentNode->m_childRows.size() > 1);
            bool matchingSiblingFound = false;

            if (hasSiblings)
            {
                // check if this node has changed to match a sibling node
                auto comparisonRow = GetComparisonRow(rowNode);
                auto& siblingVector = parentNode->m_childRows;

                for (auto siblingIter = siblingVector.begin(), endIter = siblingVector.end();
                     !matchingSiblingFound && siblingIter != endIter;
                     ++siblingIter)
                {
                    if (siblingIter->get() != rowNode)
                    {
                        // now matches sibling, join it as an entry and destroy the former node
                        auto siblingRow = GetComparisonRow(siblingIter->get());
                        if (SameRow(siblingRow, comparisonRow))
                        {
                            const bool allEntriesMatch = (*siblingIter)->m_allEntriesMatch && ValuesMatch(siblingRow, comparisonRow);
                            siblingIter->get()->AddEntry(adapterIndex, rowPath.Back().GetIndex(), allEntriesMatch);
                            matchingSiblingFound = true;
                        }
                    }
                }
            }
            if (!hasSiblings || !matchingSiblingFound && nodeWasComplete)
            {
                // there's only one entry for this node and it needs to update, replace its value with its new value
                auto nodePath = GetPathForNode(rowNode);
                AZ_Assert(!nodePath.IsEmpty(), "should never generate an empty path for a complete node!");
                NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(nodePath, GetValueHierarchyForNode(rowNode)) });
            }
        }
    }

    void RowAggregateAdapter::RemoveChildRows(Dom::Value& rowValue)
    {
        for (auto valueIter = rowValue.MutableArrayBegin(), endIter = rowValue.MutableArrayEnd(); valueIter != endIter; /*in loop*/)
        {
            if (IsRow(*valueIter))
            {
                valueIter = rowValue.ArrayErase(valueIter);
            }
            else
            {
                ++valueIter;
            }
        }
    }

    Dom::Value RowAggregateAdapter::GenerateContents()
    {
        m_builder.BeginAdapter();
        m_builder.EndAdapter();
        Dom::Value contents = m_builder.FinishAndTakeResult();

        // root node is not a row, so cannot generate a value directly. Instead, iterate its children for values
        for (auto& topLevelRow : m_rootNode->m_childRows)
        {
            Dom::Value childValue = GetValueHierarchyForNode(topLevelRow.get());
            if (!childValue.IsNull())
            {
                contents.ArrayPushBack(childValue);
            }
        }

        return contents;
    }

    Dom::Value RowAggregateAdapter::HandleMessage(const AdapterMessage& message)
    {
        AZ::Dom::Value messageResult;

        auto nodePath = message.m_messageOrigin;
        auto originalColumn = nodePath.Back().GetIndex();
        nodePath.Pop();
        auto messageNode = GetNodeAtPath(nodePath);
        AZ_Assert(messageNode, "can't find node for given AdapterMessage!");

        // check if this is from a "values differ" row
        if (!m_generateDiffRows || messageNode->m_allEntriesMatch)
        {
            /* not a "values differ" row, so it directly represents the member adapter nodes.
               Check if this message is one of the ones that the AggregateAdapter is meant to manipulate and forward to sub-adapters */
            const auto messagesToForward = GetMessagesToForward();
            if (AZStd::find(messagesToForward.begin(), messagesToForward.end(), message.m_messageName) != messagesToForward.end())
            {
                // it's a forwarded message, we need to look up the original handler for each adapter and call them individually
                for (size_t adapterIndex = 0, numAdapters = m_adapters.size(); adapterIndex < numAdapters; ++adapterIndex)
                {
                    auto attributePath = messageNode->GetPathForAdapter(adapterIndex) / originalColumn / message.m_messageName;
                    auto attributeValue = m_adapters[adapterIndex]->adapter->GetContents()[attributePath];
                    AZ_Assert(!attributeValue.IsNull(), "function attribute should exist for each adapter!");

                    auto invokeDomValueFunction = [&message](const Dom::Value& functionValue, auto&& invokeDomValueFunction) -> Dom::Value
                    {
                        Dom::Value result;
                        auto adapterFunction = BoundAdapterMessage::TryMarshalFromDom(functionValue);

                        if (adapterFunction.has_value())
                        {
                            // it's a bound adapter message, just call it, hooray!
                            result = adapterFunction.value()(message.m_messageParameters);
                        }
                        else if (functionValue.IsObject())
                        {
                            // it's an object, it should be a callable attribute
                            auto typeField = functionValue.FindMember(AZ::Attribute::GetTypeField());
                            if (typeField != functionValue.MemberEnd() && typeField->second.IsString() &&
                                typeField->second.GetString() == Attribute::GetTypeName())
                            {
                                // last chance! Check if it's an invokable Attribute
                                void* instance = AZ::Dom::Utils::ValueToTypeUnsafe<void*>(functionValue[AZ::Attribute::GetInstanceField()]);
                                AZ::Attribute* attribute =
                                    AZ::Dom::Utils::ValueToTypeUnsafe<AZ::Attribute*>(functionValue[AZ::Attribute::GetAttributeField()]);

                                const bool canInvoke = attribute->IsInvokable() && attribute->CanDomInvoke(message.m_messageParameters);
                                AZ_Assert(canInvoke, "message attribute is not invokable!");
                                if (canInvoke)
                                {
                                    result = attribute->DomInvoke(instance, message.m_messageParameters);
                                }
                            }
                        }
                        else if (functionValue.IsArray())
                        {
                            for (auto arrayIter = functionValue.ArrayBegin(), endIter = functionValue.ArrayEnd(); arrayIter != endIter;
                                 ++arrayIter)
                            {
                                // Note: currently last call in the array wins. This could be parameterized in the future if
                                // a different result is desired
                                result = invokeDomValueFunction(*arrayIter, invokeDomValueFunction);
                            }
                        }
                        else
                        {
                            // it's not a function object, it's most likely a pass-through Value, so pass it through
                            result = functionValue;
                        }
                        return result;
                    };
                    messageResult = invokeDomValueFunction(attributeValue, invokeDomValueFunction);
                }
            }
        }
        else
        {
            // not a member-adapter generated message
            auto handleEditAnyway = [&]()
            {
                // get the affected row by pulling off the trailing column index on the address
                auto rowPath = message.m_messageOrigin;
                rowPath.Pop();

                /* edit anyway forces us to act as if one representative node can talk to all sub-adapters, so from this point forward
                   we are effectively pretending that this row now matches all sub-adapters */
                messageNode->m_allEntriesMatch = true;
                NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(rowPath, GenerateAggregateRow(GetNodeAtPath(rowPath))) });
            };
            messageResult = message.Match(Nodes::GenericButton::OnActivate, handleEditAnyway);
        }
        return messageResult;
    }

    AZStd::string_view LabeledRowAggregateAdapter::GetFirstLabel(const Dom::Value& rowValue)
    {
        for (auto arrayIter = rowValue.ArrayBegin(), endIter = rowValue.ArrayEnd(); arrayIter != endIter; ++arrayIter)
        {
            auto& currChild = *arrayIter;
            if (arrayIter->GetNodeName() == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Label>())
            {
                return AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(currChild).value_or("");
            }
        }
        return AZStd::string_view();
    }

    AZStd::vector<AZ::Name> LabeledRowAggregateAdapter::GetMessagesToForward()
    {
        return { Nodes::PropertyEditor::OnChanged.GetName(),
                 Nodes::PropertyEditor::ChangeNotify.GetName(),
                 Nodes::PropertyEditor::RequestTreeUpdate.GetName(),
                 Nodes::GenericButton::OnActivate.GetName() };
    }

    Dom::Value LabeledRowAggregateAdapter::GenerateAggregateRow(AggregateNode* matchingNode)
    {
        /* use the row generated by the first matching adapter as a template, but replace its message
           handlers with a redirect to our own adapter. These will then be forwarded as needed in
           RowAggregateAdapter::HandleMessage */
        auto multiRow = GetComparisonRow(matchingNode);

        const auto editorName = AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::PropertyEditor>();
        const auto messagesToForward = GetMessagesToForward();

        for (size_t childIndex = 0, numChildren = multiRow.ArraySize(); childIndex < numChildren; ++childIndex)
        {
            auto& childValue = multiRow.MutableArrayAt(childIndex);
            if (childValue.GetNodeName() == editorName)
            {
                for (auto attributeIter = childValue.MutableMemberBegin(), attributeEnd = childValue.MutableMemberEnd();
                     attributeIter != attributeEnd;
                     ++attributeIter)
                {
                    for (const auto& messageName : messagesToForward)
                    {
                        // check if the adapter wants this message forwarded, and replace it with our own handler if it does
                        if (attributeIter->first == messageName)
                        {
                            auto nodePath = GetPathForNode(matchingNode);
                            AZ_Assert(!nodePath.IsEmpty(), "shouldn't be generating an aggregate row for a non-matching node!");
                            BoundAdapterMessage changedAttribute = { this, messageName, nodePath / childIndex, {} };
                            auto newValue = changedAttribute.MarshalToDom();
                            attributeIter->second = newValue;

                            // we've found the matching message, break out of the inner loop
                            break;
                        }
                    }
                }
            }
        }
        return multiRow;
    }

    Dom::Value LabeledRowAggregateAdapter::GenerateValuesDifferRow([[maybe_unused]] AggregateNode* mismatchNode)
    {
        m_builder.SetCurrentPath(GetPathForNode(mismatchNode));
        m_builder.BeginRow();
        m_builder.Label(GetFirstLabel(GetComparisonRow(mismatchNode)));
        m_builder.Label(AZStd::string("Values Differ"));
        m_builder.BeginPropertyEditor<Nodes::GenericButton>();
        m_builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
        m_builder.Attribute(Nodes::Button::ButtonText, AZStd::string("Edit Anyway"));
        m_builder.AddMessageHandler(this, Nodes::GenericButton::OnActivate.GetName());
        m_builder.EndPropertyEditor();
        m_builder.EndRow();
        return m_builder.FinishAndTakeResult();
    }

    bool LabeledRowAggregateAdapter::SameRow(const Dom::Value& newRow, const Dom::Value& existingRow)
    {
        auto newNodeText = GetFirstLabel(newRow);
        auto existingNodeText = GetFirstLabel(existingRow);

        return (newNodeText == existingNodeText);
    }

    bool LabeledRowAggregateAdapter::ValuesMatch(const Dom::Value& left, const Dom::Value& right)
    {
        auto getComparisonValue = [](const Dom::Value& rowValue)
        {
            if (!rowValue.IsArrayEmpty())
            {
                for (auto arrayIter = rowValue.ArrayBegin() + 1, endIter = rowValue.ArrayEnd(); arrayIter != endIter; ++arrayIter)
                {
                    auto& currChild = *arrayIter;
                    if (arrayIter->GetNodeName() == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::PropertyEditor>())
                    {
                        return AZ::Dpe::Nodes::PropertyEditor::Value.ExtractFromDomNode(currChild).value_or(
                            Dom::Value(AZ::Dom::Type::Null));
                    }
                }
            }
            return Dom::Value(AZ::Dom::Type::Null);
        };

        auto leftValue = getComparisonValue(left);
        auto rightValue = getComparisonValue(right);

        return (leftValue.GetType() != AZ::Dom::Type::Null && leftValue == rightValue);
    }
} // namespace AZ::DocumentPropertyEditor
