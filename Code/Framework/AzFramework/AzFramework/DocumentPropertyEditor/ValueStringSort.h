/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/SortAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class ValueStringSort : public RowSortAdapter
    {
    public:
        ValueStringSort();

        void SetSortAttribute(AZStd::string_view attributeName);
        void SetSortColumn(size_t sortColumn);

        struct StringSortNode : public RowSortAdapter::SortInfoNode
        {
            StringSortNode(const RowSortAdapter* owningAdapter)
                : SortInfoNode(owningAdapter)
            {
            }
            AZStd::string m_string;
        };

    protected:
        // pure virtual overrides
        SortInfoNode* NewSortInfoNode() const override;
        void CacheDomInfoForNode(const Dom::Value& domValue, SortInfoNode* sortNode) const override;
        bool LessThan(SortInfoNode* lhs, SortInfoNode* rhs) const override;

        bool m_caseSensitive = false;
        
        // sets what column and attribute to use for sorting
        size_t m_sortColumn = 0;
        Name m_sortAttributeName; // defaults to "Value"
    };
} // namespace AZ::DocumentPropertyEditor
