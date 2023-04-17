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
        // TODO: https://github.com/o3de/o3de/issues/15609
        (void)sourceAdapter;
    }

    void RowAggregateAdapter::ClearAdapters()
    {
        m_adapters.clear();
        m_rootNode = AZStd::make_unique<AggregateNode>();
        NotifyResetDocument();
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
        // TODO https://github.com/o3de/o3de/issues/15610
        (void)adapter;
    }

    void RowAggregateAdapter::HandleDomChange(DocumentAdapterPtr adapter, const Dom::Patch& patch)
    {
        (void)adapter;
        (void)patch;
        // TODO https://github.com/o3de/o3de/issues/15612
    }

    void RowAggregateAdapter::HandleDomMessage(
        [[maybe_unused]] DocumentAdapterPtr adapter,
        [[maybe_unused]] const AZ::DocumentPropertyEditor::AdapterMessage& message,
        [[maybe_unused]] Dom::Value& value)
    {
        // TODO forwarding all of these isn't desirable, test to see if we need to conditionally forward this
        // https://github.com/o3de/o3de/issues/15611
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

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::GetNodeAtAdapterPath(size_t adapterIndex, const Dom::Path& path)
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

    RowAggregateAdapter::AggregateNode* RowAggregateAdapter::GetNodeAtPath(const Dom::Path& aggregatePath)
    {
        const size_t numAdapters = m_adapters.size();
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
                if (currChild->EntryCount() == numAdapters)
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
        const size_t numAdapters = m_adapters.size();

        // verify that this and all ancestors have entries for each adapter,
        // otherwise there is no path for this node, as it won't be included in the contents
        auto* currNode = node;
        while (currNode)
        {
            if (currNode != m_rootNode.get() && currNode->EntryCount() != numAdapters)
            {
                return Dom::Path();
            }
            currNode = currNode->m_parent;
        }
        Dom::Path nodePath;
        auto addParentThenSelf = [&nodePath, numAdapters](AggregateNode* currentNode, auto&& addParentThenSelf) -> void
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
                    else if (currChild->EntryCount() == numAdapters)
                    {
                        ++currIndex;
                    }
                }
            }
        };
        addParentThenSelf(node, addParentThenSelf);
        return nodePath;
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

    Dom::Value RowAggregateAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&](const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            (void)valueFromEditor;
            (void)changeType;

            // need to make a new message origin, get the row, which is one up from this widget's path
            auto nodePath = message.m_messageOrigin;
            auto originalColumn = nodePath.Back().GetIndex();
            nodePath.Pop();
            auto messageNode = GetNodeAtPath(nodePath);

            for (size_t adapterIndex = 0, numAdapters = m_adapters.size(); adapterIndex < numAdapters; ++adapterIndex)
            {
                AdapterMessage forwardedMessage(message);
                forwardedMessage.m_messageOrigin = messageNode->GetPathForAdapter(adapterIndex) / originalColumn;
                m_adapters[adapterIndex]->adapter->SendAdapterMessage(forwardedMessage);
            }
        };

        auto handleEditAnyway = [&]()
        {
            // get the affected row by pulling off the trailing column index on the address
            auto rowPath = message.m_messageOrigin;
            rowPath.Pop();

            NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(rowPath, GenerateAggregateRow(GetNodeAtPath(rowPath))) });
        };

        return message.Match(
            Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged,
            Nodes::GenericButton::OnActivate, handleEditAnyway
        );
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
        auto multiRow = GetComparisonRow(matchingNode);

        const auto editorName = AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::PropertyEditor>();
        const auto onChangedName = Nodes::PropertyEditor::OnChanged.GetName();

        for (size_t childIndex = 0, numChildren = multiRow.ArraySize(); childIndex < numChildren; ++childIndex)
        {
            auto& childValue = multiRow.MutableArrayAt(childIndex);
            if (childValue.GetNodeName() == editorName)
            {
                for (auto attributeIter = childValue.MutableMemberBegin(), attributeEnd = childValue.MutableMemberEnd(); attributeIter != attributeEnd;
                     ++attributeIter)
                {
                    if (attributeIter->first == onChangedName)
                    {
                        childValue.RemoveMember(attributeIter);
                        auto nodePath = GetPathForNode(matchingNode);
                        AZ_Assert(!nodePath.IsEmpty(), "shouldn't be generating an aggregate row for a non-matching node!");
                        BoundAdapterMessage changedAttribute = { this, onChangedName, nodePath / childIndex, {} };
                        childValue[onChangedName] = changedAttribute.MarshalToDom();
                    }
                }
            }
        }
        return multiRow;
    }

    Dom::Value LabeledRowAggregateAdapter::GenerateValuesDifferRow([[maybe_unused]]AggregateNode* mismatchNode)
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
