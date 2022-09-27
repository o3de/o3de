/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/FilterAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    RowFilterAdapter::RowFilterAdapter() 
    {
    }

    RowFilterAdapter::~RowFilterAdapter()
    {
        delete m_root;
    }

    void RowFilterAdapter::SetSourceAdapter(DocumentAdapterPtr sourceAdapter)
    {
        m_sourceAdapter = sourceAdapter;
        m_resetHandler = DocumentAdapter::ResetEvent::Handler(
            [this]()
            {
                this->HandleReset();
            });
        m_sourceAdapter->ConnectResetHandler(m_resetHandler);

        m_changedHandler = DocumentAdapter::ChangedEvent::Handler(
            [this](const Dom::Patch& patch)
            {
                this->HandleDomChange(patch);
            });
        m_sourceAdapter->ConnectChangedHandler(m_changedHandler);

        m_domMessageHandler = AZ::DocumentPropertyEditor::DocumentAdapter::MessageEvent::Handler(
            [this](const AZ::DocumentPropertyEditor::AdapterMessage& message, AZ::Dom::Value& value)
            {
                this->HandleDomMessage(message, value);
            });
        m_sourceAdapter->ConnectMessageHandler(m_domMessageHandler);

        // populate the filter data from the source's full adapter contents, just like a reset
        HandleReset();
    }

    bool RowFilterAdapter::IsRow(const Dom::Value& domValue)
    {
        return (domValue.IsNode() && domValue.GetNodeName() == Dpe::GetNodeName<Dpe::Nodes::Row>());

    }

    bool RowFilterAdapter::IsRow(const Dom::Path& sourcePath) const
    {
        const auto& sourceRoot = m_sourceAdapter->GetContents();
        auto sourceNode = sourceRoot[sourcePath];
        return IsRow(sourceNode);
    }

    void RowFilterAdapter::SetFilterActive(bool activateFilter)
    {
        if (m_filterActive != activateFilter)
        {
            m_filterActive = activateFilter;
            NotifyResetDocument();
        }
    }

    void RowFilterAdapter::InvalidateFilter()
    {
        AZStd::function<void(MatchInfoNode*)> updateChildren = [&](MatchInfoNode* parentNode)
        {
            for (auto* childNode : parentNode->m_childMatchState)
            {
                if (childNode)
                {
                    UpdateMatchState(childNode);
                    updateChildren(childNode);
                }
            }
        };

        // recursively update each node, then its children
        updateChildren(m_root);

        if (m_filterActive)
        {
            NotifyResetDocument();
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
            // cull row children based on their MatchInfoNode
            AZStd::function<void(Dom::Value& rowValue, const MatchInfoNode* rowMatchNode)> cullUnmatchedChildRows =
            [&](Dom::Value& rowValue, const MatchInfoNode* rowMatchNode)
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
                            if (currMatch->m_matchesSelf)
                            {
                                // node matches directly. All descendants automatically match if m_includeAllMatchDescendants is set
                                if (!m_includeAllMatchDescendants)
                                {
                                    cullUnmatchedChildRows(*valueIter, *matchIter);
                                }
                            }
                            else if (!currMatch->m_matchingDescendants.empty())
                            {
                                // all nodes with matching descendants must be included so that there is a path to the matching node
                                cullUnmatchedChildRows(*valueIter, *matchIter);
                            }
                            else
                            {
                                // neither the node nor its children match, cull the value from the returned tree
                                valueIter = rowValue.ArrayErase(valueIter);
                                continue; // we've already moved valueIter forward, skip the ++valueIter below
                            }
                        }
                        ++valueIter;
                    }
                }
            };
            // filter downwards from root
            cullUnmatchedChildRows(filteredContents, m_root);
        }
        return filteredContents;
    }

    void RowFilterAdapter::HandleReset()
    {
        delete m_root;
        m_root = NewMatchInfoNode(nullptr);
        const auto& sourceContents = m_sourceAdapter->GetContents();

        // we can assume all direct children of an adapter must be rows; populate each of them
        for (size_t topIndex = 0; topIndex < sourceContents.ArraySize(); ++topIndex)
        {
            PopulateNodesAtPath(Dom::Path({Dom::PathEntry(topIndex)}), false);
        }
        NotifyResetDocument(DocumentResetType::HardReset);
    }

    void RowFilterAdapter::HandleDomChange(const Dom::Patch& patch)
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
        NotifyResetDocument();
    }

    void RowFilterAdapter::HandleDomMessage(const AZ::DocumentPropertyEditor::AdapterMessage& message,[[maybe_unused]] Dom::Value& value)
    {
        // forward all messages unaltered
        DocumentAdapter::SendAdapterMessage(message);
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
                        matchState->m_childMatchState[arrayIndex] = NewMatchInfoNode(matchState);
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
                    addedChild = NewMatchInfoNode(parentMatchState);
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
                addedChild = NewMatchInfoNode(parentMatchState);
                parentMatchState->m_childMatchState.insert(
                    parentMatchState->m_childMatchState.begin() + newRowIndex,
                    addedChild);
            }

            // recursively add descendants and cache their match status
            const auto& sourceContents = m_sourceAdapter->GetContents();
            recursivelyRegenerateMatches(addedChild, sourceContents[sourcePath]);
        }
    }

    void RowFilterAdapter::UpdateMatchState(MatchInfoNode* rowState)
    {
        auto includeInFilter = [](MatchInfoNode* node) -> bool
        {
            return (node->m_matchesSelf || !node->m_matchingDescendants.empty());
        };
        bool usedToMatch = includeInFilter(rowState);

        rowState->m_matchesSelf = MatchesFilter(rowState); //update own match state

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

        // update each ancestor until updating their m_matchingDescendants doesn't change their inclusion state
        MatchInfoNode* currState = rowState;
        bool nowMatches = includeInFilter(currState);
        while (currState->m_parentNode && nowMatches != usedToMatch)
        {
            usedToMatch = includeInFilter(currState->m_parentNode);
            if (nowMatches)
            {
                currState->m_parentNode->m_matchingDescendants.insert(currState);
            }
            else
            {
                currState->m_parentNode->m_matchingDescendants.erase(currState);
            }
            nowMatches = includeInFilter(currState->m_parentNode);
            currState = currState->m_parentNode;
        }
    }

    Dom::Path RowFilterAdapter::GetRowPath(const Dom::Path& sourcePath) const
    {
        Dom::Path rowPath;
        const auto& contents = m_sourceAdapter->GetContents();
        const auto* currDomValue = &contents;
        for (const auto& pathEntry : sourcePath)
        {
            currDomValue = &(*currDomValue)[pathEntry];
            if (IsRow(*currDomValue))
            {
                rowPath.Push(pathEntry);
            }
        }
        return rowPath;
    }

    ValueStringFilter::ValueStringFilter()
        : RowFilterAdapter()
    {
    }

    void ValueStringFilter::SetFilterString(QString filterString)
    {
        if (m_filterString != filterString)
        {
            m_filterString = AZStd::move(filterString);
            InvalidateFilter();
            SetFilterActive(!m_filterString.isEmpty());
        }
    }

    RowFilterAdapter::MatchInfoNode* ValueStringFilter::NewMatchInfoNode(MatchInfoNode* parentRow) const
    {
        return new StringMatchNode(parentRow);
    }

    void ValueStringFilter::CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const
    {
        auto actualNode = static_cast<StringMatchNode*>(matchNode);
        const bool nodeIsRow = IsRow(domValue);
        AZ_Assert(nodeIsRow, "Only row nodes should be cached by a RowFilterAdapter");
        if (nodeIsRow)
        {
            actualNode->m_matchableDomTerms.clear();
            for (auto childIter = domValue.ArrayBegin(), endIter = domValue.ArrayEnd(); childIter != endIter; ++childIter)
            {
                auto& currChild = *childIter;
                if (currChild.IsNode())
                {
                    auto childName = currChild.GetNodeName();
                    if (childName != Dpe::GetNodeName<Dpe::Nodes::Row>()) // don't cache child rows, they have they're own entries
                    {
                        static const Name valueName("Value");
                        auto foundValue = currChild.FindMember(valueName);
                        if (foundValue != currChild.MemberEnd())
                        {
                            actualNode->AddStringifyValue(foundValue->second);
                        }

                        if (m_includeDescriptions)
                        {
                            static const Name descriptionName("Description");
                            auto foundDescription = currChild.FindMember(descriptionName);
                            if (foundDescription != currChild.MemberEnd())
                            {
                                actualNode->AddStringifyValue(foundDescription->second);
                            }
                        }
                    }
                }
            }
        }
    }

    bool ValueStringFilter::MatchesFilter(MatchInfoNode* matchNode) const
    {
        auto actualNode = static_cast<StringMatchNode*>(matchNode);
        return (m_filterString.isEmpty() || actualNode->m_matchableDomTerms.contains(m_filterString, Qt::CaseInsensitive));
    }

    void ValueStringFilter::StringMatchNode::AddStringifyValue(const Dom::Value& domValue)
    {
        QString stringifiedValue;

        if (domValue.IsNull())
        {
            stringifiedValue = QStringLiteral("null");
        }
        else if (domValue.IsBool())
        {
            stringifiedValue = (domValue.GetBool() ? QStringLiteral("true") : QStringLiteral("false"));
        }
        else if (domValue.IsInt())
        {
            stringifiedValue = QString::number(domValue.GetInt64());
        }
        else if (domValue.IsUint())
        {
            stringifiedValue = QString::number(domValue.GetUint64());
        }
        else if (domValue.IsDouble())
        {
            stringifiedValue = QString::number(domValue.GetDouble());
        }
        else if (domValue.IsString())
        {
            AZStd::string_view stringView = domValue.GetString();
            stringifiedValue = QString::fromUtf8(stringView.data(), aznumeric_cast<int>(stringView.size()));
        }

        if (!stringifiedValue.isEmpty())
        {
            if (!m_matchableDomTerms.isEmpty())
            {
                m_matchableDomTerms.append(QChar::Space);
            }
            m_matchableDomTerms.append(stringifiedValue);
        }
    }
} // namespace AZ::DocumentPropertyEditor
