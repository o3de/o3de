/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/ValueStringSort.h>

namespace AZ::DocumentPropertyEditor
{
    ValueStringSort::ValueStringSort()
        : RowSortAdapter()
        , m_sortAttributeName(Name::FromStringLiteral("Value", AZ::Interface<AZ::NameDictionary>::Get()))
    {
    }

    void ValueStringSort::SetSortAttribute(AZStd::string_view attributeName)
    {
        m_sortAttributeName = Name::FromStringLiteral(attributeName, AZ::Interface<AZ::NameDictionary>::Get());
        InvalidateSort();
    }

    void ValueStringSort::SetSortColumn(size_t sortColumn)
    {
        m_sortColumn = sortColumn;
        InvalidateSort();
    }

    RowSortAdapter::SortInfoNode* ValueStringSort::NewSortInfoNode() const
    {
        return new StringSortNode(this);
    }

    void ValueStringSort::CacheDomInfoForNode(const Dom::Value& domValue, RowSortAdapter::SortInfoNode* sortNode) const
    {
        auto actualNode = static_cast<StringSortNode*>(sortNode);
        const bool nodeIsRow = IsRow(domValue);
        AZ_Assert(nodeIsRow, "Only row nodes should be cached by a RowFilterAdapter");
        if (nodeIsRow)
        {
            auto getValueForColumn = [](const Dom::Value& rowValue, size_t column) -> const Dom::Value*
            {
                int atColumn = 0;
                for (auto childIter = rowValue.ArrayBegin(), endIter = rowValue.ArrayEnd(); childIter != endIter; ++childIter)
                {
                    if (!IsRow(*childIter))
                    {
                        // non-row children are, by definition, column children
                        if (atColumn == column)
                        {
                            return &(*childIter);
                        }
                        else
                        {
                            // not at the right column number yet
                            ++atColumn;
                        }
                    }
                }
                return nullptr;
            };
            auto sortValue = getValueForColumn(domValue, m_sortColumn);
            if (sortValue)
            {
                auto foundValue = sortValue->FindMember(m_sortAttributeName);
                if (foundValue != sortValue->MemberEnd())
                {
                    auto attributeValue = foundValue->second;

                    if (attributeValue.IsNull())
                    {
                        actualNode->m_string = "null";
                    }
                    else if (attributeValue.IsBool())
                    {
                        actualNode->m_string = (attributeValue.GetBool() ? "true" : "false");
                    }
                    else if (attributeValue.IsInt())
                    {
                        actualNode->m_string = AZStd::to_string(attributeValue.GetInt64());
                    }
                    else if (attributeValue.IsUint())
                    {
                        actualNode->m_string = AZStd::to_string(attributeValue.GetUint64());
                    }
                    else if (attributeValue.IsDouble())
                    {
                        actualNode->m_string = AZStd::to_string(attributeValue.GetDouble());
                    }
                    else if (attributeValue.IsString())
                    {
                        AZStd::string_view stringView = attributeValue.GetString();
                        actualNode->m_string = stringView;

                        if (!m_caseSensitive)
                        {
                            AZStd::to_lower(actualNode->m_string.begin(), actualNode->m_string.end());
                        }
                    }
                }
            }
        }
    }

    bool ValueStringSort::LessThan(SortInfoNode* lhs, SortInfoNode* rhs) const
    {
        auto leftNode = static_cast<StringSortNode*>(lhs);
        auto rightNode = static_cast<StringSortNode*>(rhs);
        bool isLess = AZStd::lexicographical_compare(
            leftNode->m_string.begin(), leftNode->m_string.end(),
            rightNode->m_string.begin(), rightNode->m_string.end());

        if (!isLess)
        {
            // it's not less, but we need to make sure there aren't any ties
            const bool isGreater = AZStd::lexicographical_compare(
                rightNode->m_string.begin(), rightNode->m_string.end(), leftNode->m_string.begin(), leftNode->m_string.end());

            if (!isGreater)
            {
                // it's neither less or greater, use the base implementation (initial domIndex) to break the tie)
                isLess = RowSortAdapter::LessThan(lhs, rhs);
            }
        }
        return isLess;
    }
} // namespace AZ::DocumentPropertyEditor
