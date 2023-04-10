/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/FilterAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class ValueStringFilter : public RowFilterAdapter
    {
    public:
        ValueStringFilter();
        void SetFilterString(AZStd::string filterString);

    protected:
        // pure virtual overrides
        MatchInfoNode* NewMatchInfoNode() const override;
        void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const override;
        bool MatchesFilter(MatchInfoNode* matchNode) const override;

        struct StringMatchNode : public RowFilterAdapter::MatchInfoNode
        {
            void AddStringifyValue(const Dom::Value& domValue);

            AZStd::string m_matchableDomTerms;
        };

        bool m_includeDescriptions = true;
        AZStd::string m_filterString;
    };
} // namespace AZ::DocumentPropertyEditor
