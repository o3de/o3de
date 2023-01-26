/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/set.h>
#include <AzFramework/DocumentPropertyEditor/MetaAdapter.h>

namespace AZ::DocumentPropertyEditor
{

    class RowSortAdapter : public MetaAdapter
    {
    public:
        virtual ~RowSortAdapter();
        void SetSortActive(bool active);
        void InvalidateSort();

        // MetaAdapter overrides
        Dom::Path MapFromSourcePath(const Dom::Path& sourcePath) const override;
        Dom::Path MapToSourcePath(const Dom::Path& filterPath) const override;

        Dom::Path MapPath(const Dom::Path& sourcePath, bool mapToSource) const;

    protected:
        Dom::Value GenerateContents() override;

        struct SortInfoNode;

        static bool indexLessThan(const AZStd::unique_ptr<SortInfoNode>& lhs, const AZStd::unique_ptr<SortInfoNode>& rhs)
        {
            return lhs->m_domIndex < rhs->m_domIndex;
        }

        struct SortInfoNode
        {
            using AdapterSortType = std::function<bool(SortInfoNode*, SortInfoNode*)>;
            using IndexSortType = std::function<bool(const AZStd::unique_ptr<SortInfoNode>&, const AZStd::unique_ptr<SortInfoNode>&)>;

            virtual ~SortInfoNode()
            {
            }

            size_t m_domIndex;

            //! holds the row childNodes in DOM order
            AZStd::set<AZStd::unique_ptr<SortInfoNode>, IndexSortType> m_indexSortedChildren;

            //! holds a sorted multiset of the above children as defined by their RowSortAdapter::lessThan
            AZStd::multiset<SortInfoNode*, AdapterSortType> m_adapterSortedChildren;

        protected:
            SortInfoNode(const RowSortAdapter* owningAdapter)
                : m_indexSortedChildren(&indexLessThan)
                , m_adapterSortedChildren(
                      [owningAdapter](SortInfoNode* lhs, SortInfoNode* rhs)
                      {
                          return owningAdapter->LessThan(lhs, rhs);
                      })
            {
            }

            // when children get inserted, ++index of all subsequent entries in m_childSortInfo,
            // as returned from the iterator returned during insert!
        };
        AZStd::unique_ptr<SortInfoNode> m_rootNode;

        // pure virtual methods for new RowSortAdapter
        virtual SortInfoNode* NewSortInfoNode() const = 0;
        virtual void CacheDomInfoForNode(const Dom::Value& domValue, SortInfoNode* sortNode) const = 0;
        virtual bool LessThan(SortInfoNode* lhs, SortInfoNode* rhs) const = 0;

        // handlers for source adapter's messages
        void HandleReset() override;
        void HandleDomChange(const Dom::Patch& patch) override;

        void GenerateFullTree();
        void PopulateChildren(const Dom::Value& value, SortInfoNode* sortInfo);

        Dom::Value GetSortedValue(const Dom::Value& sourceValue, SortInfoNode* sortInfo);
        void ResortRow(Dom::Path sortedPath);

        bool m_sortActive = true;
        bool m_reverseSort = false;
    };

} // namespace AZ::DocumentPropertyEditor
