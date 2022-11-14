/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/ValueStringFilter.h>

namespace AZ::DocumentPropertyEditor
{
    ValueStringFilter::ValueStringFilter()
        : RowFilterAdapter()
    {
    }

    void ValueStringFilter::SetFilterString(AZStd::string filterString)
    {
        AZStd::to_lower(filterString.begin(), filterString.end());
        if (m_filterString != filterString)
        {
            m_filterString = AZStd::move(filterString);

            if (m_filterActive)
            {
                if (m_filterString.empty())
                {
                    SetFilterActive(false);
                }
                else
                {
                    InvalidateFilter();
                }
            }
            else if (!m_filterString.empty())
            {
                SetFilterActive(true);
            }
        }
    }

    RowFilterAdapter::MatchInfoNode* ValueStringFilter::NewMatchInfoNode() const
    {
        return new StringMatchNode();
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
                        static const Name valueName = AZ::Dpe::Nodes::PropertyEditor::Value.GetName();
                        auto foundValue = currChild.FindMember(valueName);
                        if (foundValue != currChild.MemberEnd())
                        {
                            actualNode->AddStringifyValue(foundValue->second);
                        }

                        if (m_includeDescriptions)
                        {
                            static const Name descriptionName = AZ::Dpe::Nodes::PropertyEditor::Description.GetName();
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
        return (m_filterString.empty() || actualNode->m_matchableDomTerms.contains(m_filterString));
    }

    void ValueStringFilter::StringMatchNode::AddStringifyValue(const Dom::Value& domValue)
    {
        AZStd::string stringifiedValue;

        if (domValue.IsNull())
        {
            stringifiedValue = "null";
        }
        else if (domValue.IsBool())
        {
            stringifiedValue = (domValue.GetBool() ? "true" : "false");
        }
        else if (domValue.IsInt())
        {
            stringifiedValue = AZStd::to_string(domValue.GetInt64());
        }
        else if (domValue.IsUint())
        {
            stringifiedValue = AZStd::to_string(domValue.GetUint64());
        }
        else if (domValue.IsDouble())
        {
            stringifiedValue = AZStd::to_string(domValue.GetDouble());
        }
        else if (domValue.IsString())
        {
            AZStd::string_view stringView = domValue.GetString();
            stringifiedValue = stringView;
            AZStd::to_lower(stringifiedValue.begin(), stringifiedValue.end());
        }

        if (!stringifiedValue.empty())
        {
            if (!m_matchableDomTerms.empty())
            {
                m_matchableDomTerms.append(" ");
            }
            m_matchableDomTerms.append(stringifiedValue);
        }
    }
} // namespace AZ::DocumentPropertyEditor
