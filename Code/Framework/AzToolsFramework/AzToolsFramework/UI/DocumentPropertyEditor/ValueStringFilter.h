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

        struct StringMatchNode : public RowFilterAdapter::MatchInfoNode
        {
            void AddStringifyValue(const Dom::Value& domValue);

            AZStd::string m_matchableDomTerms;
        };

    protected:
        // pure virtual overrides
        virtual MatchInfoNode* NewMatchInfoNode() const override;
        virtual void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const override;
        virtual bool MatchesFilter(MatchInfoNode* matchNode) const override;

        bool m_includeDescriptions = true;
        AZStd::string m_filterString;
    };
} // namespace AZ::DocumentPropertyEditor
