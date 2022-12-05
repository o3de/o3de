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
        // capture the acutal adapter, not just the index, as adding or removing adapters could change the index
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

        PopulateNodesForAdapter(m_adapters.size() - 1);
    }

    void RowAggregateAdapter::RemoveAdapter(DocumentAdapterPtr sourceAdapter)
    {
        // TODO
        (void)sourceAdapter;
    }

    void RowAggregateAdapter::ClearAdapters()
    {
        m_adapters.clear();
        m_rootNode = AZStd::make_unique<AggregateNode>();
    }

    bool RowAggregateAdapter::IsRow(const Dom::Value& domValue)
    {
        return (domValue.IsNode() && domValue.GetNodeName() == Dpe::GetNodeName<Dpe::Nodes::Row>());
    }

    bool RowAggregateAdapter::AggregateNode::HasEntryForAdapter(size_t adapterIndex)
    {
        return (m_pathEntries.size() > adapterIndex && m_pathEntries[adapterIndex] != InvalidEntry);
    }

    Dom::Path RowAggregateAdapter::AggregateNode::GetPathForAdapter(size_t adapterIndex)
    {
        Dom::Path pathForAdapter;
        auto addParentThenSelf = [&pathForAdapter, adapterIndex](AggregateNode* currentNode, auto&& addParentThenSelf) -> void
        {
            if (currentNode->m_parent)
            {
                addParentThenSelf(currentNode->m_parent, addParentThenSelf);
            }
            if (adapterIndex < currentNode->m_pathEntries.size())
            {
                const size_t pathEntry = currentNode->m_pathEntries[adapterIndex];
                if (pathEntry != InvalidEntry)
                {
                    pathForAdapter.Push(pathEntry);
                }
            }
        };
        addParentThenSelf(this, addParentThenSelf);
        return pathForAdapter;
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
            m_parent->m_pathIndexToChildMaps[adapterIndex][pathEntryIndex] = this;
        }
    }

    size_t RowAggregateAdapter::AggregateNode::EntryCount()
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

    void RowAggregateAdapter::HandleAdapterReset(DocumentAdapterPtr adapter)
    {
        // TODO
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
                auto* nodeAtPath = GetNodeAtPath(adapterIndex, patchPath);
                if (nodeAtPath)
                {
                    // <apm> remove this entry from the node, update the "values differ"
                }
                else
                {
                    // if there's no node at that path, it was a column entry
                    auto parentPath = patchPath;
                    parentPath.Pop();
                    auto* rowParentNode = GetNodeAtPath(adapterIndex, parentPath);
                    (void)rowParentNode;
                    /* <apm> see if the entry for rowParentNode at adapterIndex still is SameRow (NB: if adapterIndex is 0, you need to use a different GetComparisonRow),
                    if not, remove it and place it where it actually goes. If yes, update the node's "values differ" status */
                    // <apm> it's almost certainly simpler to remove this parent node and repopulate to see if it ends up in the same bucket, but need to track exactly
                    // what's been updated since last frame for patching purposes
                }
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
            {
                // <apm>
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                // <apm>
            }
        }

        // <apm> adds or removes cause all subsequent entries to change index. Update m_pathIndexToChildMaps of parent *and* m_pathEntries
        // of child
        // <apm> need to only change out column children when swapping "values differ" state, so that we don't have to cull and re-add all row children
        // <apm> it's possible / likely that a node may be removed then added in very quick succession, as more than one adapter adds or removes children
    }

    void RowAggregateAdapter::HandleDomMessage(
        [[maybe_unused]] DocumentAdapterPtr adapter,
        const AZ::DocumentPropertyEditor::AdapterMessage& message,
        [[maybe_unused]] Dom::Value& value)
    {
        // forward all messages unaltered... for now
        DocumentAdapter::SendAdapterMessage(message);
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

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::GetNodeAtPath(size_t adapterIndex, const Dom::Path& path)
    {
        AggregateNode* currNode = m_rootNode.get();
        if (path.Size() < 1)
        {
            // path is empty, return the root node
            return currNode;
        }

        for (const auto& pathEntry : path)
        {
            if (!pathEntry.IsIndex())
            {
                // this path includes a non-index entry, and is therefore not a row
                return nullptr;
            }
            const auto index = pathEntry.GetIndex();
            auto& pathMap = currNode->m_pathIndexToChildMaps[adapterIndex];
            if (auto foundIter = pathMap.find(index); foundIter != pathMap.end())
            {
                currNode = foundIter->second;
            }
            else
            {
                // nothing for that path entry
                return nullptr;
            }
        }

        return currNode;
    }

    Dom::Value RowAggregateAdapter::GetComparisonRow(AggregateNode* aggregateNode)
    {
        // for now, return first entry from the map, assert if map is empty
        auto removeChildRows = [](Dom::Value& rowValue)
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
        };

        for (size_t adapterIndex = 0, numAdapters = m_adapters.size(); adapterIndex < numAdapters; ++adapterIndex)
        {
            Dom::Path pathToComparisonValue = aggregateNode->GetPathForAdapter(adapterIndex);
            if (!pathToComparisonValue.IsEmpty())
            {
                auto comparisonRow = m_adapters[adapterIndex]->adapter->GetContents()[pathToComparisonValue];
                removeChildRows(comparisonRow);
                return comparisonRow;
            }
        }
        AZ_Warning(
            "RowAggregateAdapter",
            false,
            "no valid path found in AggregateNode -- there either should be one, or this entry shouldn't exist!");
        return Dom::Value();
    }

    void RowAggregateAdapter::PopulateNodesForAdapter(size_t adapterIndex)
    {
        auto sourceAdapter = m_adapters[adapterIndex]->adapter;
        const auto& sourceContents = sourceAdapter->GetContents();
        PopulateChildren(adapterIndex, sourceContents, m_rootNode.get());
    }

    void RowAggregateAdapter::PopulateChildren(size_t adapterIndex, const Dom::Value& parentValue, AggregateNode* parentNode)
    {
        // go through each DOM child of parentValue, ignoring non-rows
        const auto numChildren = parentValue.ArraySize();
        for (size_t childIndex = 0; childIndex < numChildren; ++childIndex)
        {
            const auto& childValue = parentValue[childIndex];
            if (IsRow(childValue))
            {
                AggregateNode* addedToNode = nullptr;

                // check each existing child to see if we belong there.
                for (auto matchIter = parentNode->m_childRows.begin(), endIter = parentNode->m_childRows.end();
                     !addedToNode && matchIter != endIter;
                     ++matchIter)
                {
                    AggregateNode* possibleMatch = matchIter->get();

                    // make sure there isn't already an entry for this adapter. This can matter in weird
                    // edge cases where multiple rows can match, like in multisets
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
                }
            }
        }
    }

    Dom::Value RowAggregateAdapter::GenerateContents()
    {
        m_builder.BeginAdapter();
        m_builder.EndAdapter();
        Dom::Value contents = m_builder.FinishAndTakeResult();
        const size_t numAdapters = m_adapters.size();
        auto AddChildrenToValue = [=](AggregateNode* parentNode, Dom::Value& value, auto&& AddChildrenToValue) -> void
        {
            for (auto& currChild : parentNode->m_childRows)
            {
                if (currChild->EntryCount() == numAdapters)
                {
                    auto generatedValue =
                        (currChild->m_allEntriesMatch ? GenerateAggregateRow(currChild.get()) : GenerateValuesDifferRow(currChild.get()));
                    
                    AddChildrenToValue(currChild.get(), generatedValue, AddChildrenToValue);
                    value.ArrayPushBack(generatedValue);
                }
            }
        };

        AddChildrenToValue(m_rootNode.get(), contents, AddChildrenToValue);
        return contents;
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

    Dom::Value LabeledRowAggregateAdapter::GenerateAggregateRow(AggregateNode* matchingNode)
    {
        return GetComparisonRow(matchingNode);
    }

    Dom::Value LabeledRowAggregateAdapter::GenerateValuesDifferRow([[maybe_unused]]AggregateNode* mismatchNode)
    {
        m_builder.BeginRow();
        m_builder.Label(GetFirstLabel(GetComparisonRow(mismatchNode)));
        m_builder.Label(AZStd::string("Values Differ"));
        m_builder.BeginPropertyEditor<Nodes::Button>();
        m_builder.Attribute(Nodes::PropertyEditor::SharePriorColumn, true);
        m_builder.Attribute(Nodes::Button::ButtonText, AZStd::string("Edit Anyway"));
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
        auto getLastHandlerValue = [](const Dom::Value& rowValue)
        {
            if (!rowValue.IsArrayEmpty())
            {
                for (auto arrayIter = rowValue.ArrayEnd() - 1, endIter = rowValue.ArrayBegin(); arrayIter != endIter; --arrayIter)
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

        auto leftValue = getLastHandlerValue(left);
        auto rightValue = getLastHandlerValue(right);

        /* TODO: use actually compare these values. Currently this doesn't work because values aren't serialized into the DOM, so
                 the below comparison doesn't work for pointer types, opaque types, container rows, etc
        return (leftValue.GetType() != AZ::Dom::Type::Null && leftValue == rightValue); */
        return true;
    }


    /* notes:

    - A node is only considered matching if its number of Values equals the number of adapters
    - If all the nodes in a given set have all column children value matching, the full row is passed through,
      otherwise, the column children are replaced with a label saying, "Values differ", and a button saying "Edit anyway"
      if Edit anyyway is pressed, the first value in the set is passed through, but its editor is set to change the values of all nodes
    - the types of all nodes must match to be considered to be editable together

    TBD:
    - need to figure out how to reliably apply the edits of one PropertyEditor to all represented nodes
    - need to handle nodes with children that could differ on subsequent levels, like:
    - A has vector B, which has C, D, E children;
    - A' has vector B', which has D, E, F children;
    */

} // namespace AZ::DocumentPropertyEditor
