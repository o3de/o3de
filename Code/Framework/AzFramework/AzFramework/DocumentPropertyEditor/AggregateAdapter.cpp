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
        // Note that many types of patch operations are still unsupported at this time
        // Please see: https://github.com/o3de/o3de/issues/15612
        const auto adapterIndex = GetIndexForAdapter(adapter);

        for (auto operationIterator = patch.begin(), endIterator = patch.end(); operationIterator != endIterator; ++operationIterator)
        {
            const auto& patchPath = operationIterator->GetDestinationPath();
            if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Remove)
            {
                auto* nodeAtPath = GetNodeAtAdapterPath(adapterIndex, patchPath);
                if (nodeAtPath)
                {
                    // TODO: remove this entry from the node, update the "values differ"
                }
                else
                {
                    // if there's no node at that path, it was a column entry
                    auto parentPath = patchPath;
                    parentPath.Pop();
                    auto* rowParentNode = GetNodeAtAdapterPath(adapterIndex, parentPath);
                    (void)rowParentNode;
                    /* TODO:
                    - see if the entry for rowParentNode at adapterIndex still is SameRow (NB: if adapterIndex is 0, you need to use
                      a different GetComparisonRow), if not, remove it and place it where it actually goes. If yes, update the node's
                    "values differ" status
                    - Suggestion: it's almost certainly simpler to remove this parent node and repopulate to see if it ends up in the same
                      bucket, but need to track exactly what's been updated since last frame for patching purposes */
                }
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Replace)
            {
                // nodes represent entire aggregate rows, so if there's a node, it a replace for a full row
                auto* nodeAtPath = GetNodeAtAdapterPath(adapterIndex, patchPath);
                if (!nodeAtPath)
                {
                    if (!IsRow(operationIterator->GetValue()))
                    {
                        // neither the child being replaced nor the replacement is a row, we just need
                        // to make sure the new value is still the SameRow and update its matching state
                        auto rowPath = patchPath;
                        AggregateNode* ancestorRowNode = nullptr;

                        // find the first row node in this patch's ancestry, if it exists
                        do
                        {
                            rowPath.Pop();
                            ancestorRowNode = GetNodeAtAdapterPath(adapterIndex, rowPath);
                        } while (!rowPath.IsEmpty() && !ancestorRowNode);

                        // if there's a node for this value, and it's not the only entry in the node,
                        // we need to check if it still belongs with the other entries
                        if (ancestorRowNode)
                        {
                            if (ancestorRowNode->EntryCount() > 1)
                            {
                                auto comparisonRow = GetComparisonRow(ancestorRowNode, adapterIndex);
                                AZ_Assert(
                                    !comparisonRow.IsNull(),
                                    "there should always be a valid comparison value for a node with more than one entry");

                                auto adapterRow = adapter->GetContents()[rowPath];
                                if (SameRow(comparisonRow, adapterRow))
                                {
                                    bool entriesMatch = true;
                                    for (size_t index = 0, numEntries = ancestorRowNode->m_pathEntries.size();
                                         index < numEntries && entriesMatch;
                                         ++index)
                                    {
                                        if (index != adapterIndex && ancestorRowNode->m_pathEntries[index] != AggregateNode::InvalidEntry)
                                        {
                                            auto currAdapterPath = ancestorRowNode->GetPathForAdapter(index);
                                            AZ_Assert(
                                                !currAdapterPath.IsEmpty(),
                                                "ancestorRowNode had an entry for this adapter -- it must have a valid path!");
                                            auto currRowValue = m_adapters[index]->adapter->GetContents()[currAdapterPath];
                                            entriesMatch = ValuesMatch(adapterRow, currRowValue);
                                        }
                                    }

                                    if (entriesMatch != ancestorRowNode->m_allEntriesMatch)
                                    {
                                        // m_allEntriesMatch has changed for this node. If this node is included in the current multi-edit,
                                        // issue a replacement patch for its row value
                                        ancestorRowNode->m_allEntriesMatch = entriesMatch;

                                        auto nodePath = GetPathForNode(ancestorRowNode);
                                        if (!nodePath.IsEmpty())
                                        {
                                            NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(
                                                nodePath,
                                                (entriesMatch || !m_generateDiffRows ? GenerateAggregateRow(ancestorRowNode)
                                                                                     : GenerateValuesDifferRow(ancestorRowNode))) });
                                        }
                                    }
                                }
                                else
                                {
                                    // TODO: no longer the same row, check for a matching sibling,
                                    // and if there isn't one, split off a new node
                                    AZ_Assert(0, "replace operation morphing node is not supported yet!");
                                }
                            }
                            else
                            {
                                // TODO: handle case where there's only one entry in this node, but it might've changed.
                                // need to check if it has changed to match a parallel node, in which case it should join
                                // that node, and this one should be destroyed.
                                AZ_Assert(0, "replace operation changing value of single entry node is not supported yet!");
                            }
                        }
                    }
                }
                else
                {
                    // TODO: handle row being replaced
                    AZ_Assert(0, "AggregateAdapter: row replace operation is not supported yet!");
                }
            }
            else if (operationIterator->GetType() == AZ::Dom::PatchOperation::Type::Add)
            {
                auto destinationPath = operationIterator->GetDestinationPath();
                auto lastPathEntry = destinationPath.Back();
                destinationPath.Pop();
                auto parentNode = GetNodeAtAdapterPath(adapterIndex, destinationPath);
                AZ_Assert(parentNode, "can't find node to add to!");
                if (parentNode)
                {
                    if (IsRow(operationIterator->GetValue()))
                    {
                        // adding a full row
                        AZ_Assert(lastPathEntry.IsIndex(), "new addition is a row, it must be addressed by index!");
                        auto childIndex = lastPathEntry.GetIndex();
                        auto* aggregateNode = AddChildRow(adapterIndex, parentNode, operationIterator->GetValue(), childIndex);
                        if (aggregateNode && aggregateNode->EntryCount() == m_adapters.size())
                        {
                            // the aggregate node that this add affected is now complete
                            NotifyContentsChanged(
                                { Dom::PatchOperation::AddOperation(GetPathForNode(aggregateNode), GetValueHierarchyForNode(aggregateNode)) });
                        }
                    }
                    else
                    {
                        // not a full row - see if adding this child causes its parent to no longer match its other entries
                        AZ_Assert(0, "addition of non-row is not yet supported!");
                    }
                }
            }
        }

        /* TODO: adds or removes cause all subsequent entries to change index. Update m_pathIndexToChildMaps of parent *and* m_pathEntries
           of child. Note! We need to only change out column children when swapping "values differ" state, so that we don't have to cull and
           re-add all row children. It's possible / likely that a node may be removed then added in very quick succession, as more than
           one adapter adds or removes children */
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
            if (!pathEntry.IsIndex() || adapterIndex >= currNode->m_pathIndexToChildMaps.size())
            {
                // this path includes a non-index entry or an index out of bounds, and is therefore invalid
                return nullptr;
            }
            auto& pathMap = currNode->m_pathIndexToChildMaps[adapterIndex];
            const auto index = pathEntry.GetIndex();
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

    Dom::Value RowAggregateAdapter::GetComparisonRow(AggregateNode* aggregateNode, size_t omitAdapterIndex)
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
            if (adapterIndex != omitAdapterIndex) // don't generate a value from the adapter at omitAdapterIndex, if it exists
            {
                Dom::Path pathToComparisonValue = aggregateNode->GetPathForAdapter(adapterIndex);
                if (!pathToComparisonValue.IsEmpty())
                {
                    auto comparisonRow = m_adapters[adapterIndex]->adapter->GetContents()[pathToComparisonValue];
                    removeChildRows(comparisonRow);
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
        if (aggregateNode && aggregateNode->EntryCount() == m_adapters.size())
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
                AZStd::move(childlessRow.MutableArrayBegin(), childlessRow.MutableArrayEnd(), AZStd::back_inserter(returnValue.GetMutableArray()));
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
        size_t adapterIndex, AggregateNode* parentNode, const Dom::Value& childValue, size_t childIndex)
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
                AddChildRow(adapterIndex, parentNode, childValue, childIndex);
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

        // check if this message is one of the ones that the AggregateAdapter is meant to manipulate and forward to sub-adapters
        const auto messagesToForward = GetMessagesToForward();
        if (AZStd::find(messagesToForward.begin(), messagesToForward.end(), message.m_messageName) != messagesToForward.end())
        {
            auto nodePath = message.m_messageOrigin;
            auto originalColumn = nodePath.Back().GetIndex();
            nodePath.Pop();
            auto messageNode = GetNodeAtPath(nodePath);
            AZ_Assert(messageNode, "can't find node for given AdapterMessage!");

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
        else
        {
            auto handleEditAnyway = [&]()
            {
                // get the affected row by pulling off the trailing column index on the address
                auto rowPath = message.m_messageOrigin;
                rowPath.Pop();

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
