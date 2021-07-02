/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_unordered_set.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QSortFilterProxyModel>
#include <QSharedPointer>
#include <QCollator>
#endif
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel
            : public QSortFilterProxyModel
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserFilterModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserFilterModel(QObject* parent = nullptr);
            ~AssetBrowserFilterModel() override;

            //asset type filtering
            void SetFilter(FilterConstType filter);
            void FilterUpdatedSlotImmediate();

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;

        Q_SIGNALS:
            void filterChanged();

            //////////////////////////////////////////////////////////////////////////
            //QSortFilterProxyModel
        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            bool filterAcceptsColumn(int source_column, const QModelIndex& /*source_parent*/) const override;
            bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
            //////////////////////////////////////////////////////////////////////////

        public Q_SLOTS:
            void filterUpdatedSlot();

        protected:
            //set for filtering columns
            //if the column is in the set the column is not filtered and is shown
            AZStd::fixed_unordered_set<int, 3, static_cast<int>(AssetBrowserEntry::Column::Count)> m_showColumn;
            bool m_alreadyRecomputingFilters = false;
            //asset source name match filter
            FilterConstType m_filter;
            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
            QWeakPointer<const StringFilter> m_stringFilter;
            QWeakPointer<const CompositeFilter> m_assetTypeFilter;
            QCollator m_collator;  // cache the collator as its somewhat expensive to constantly create and destroy one.
            AZ_POP_DISABLE_WARNING
            bool m_invalidateFilter = false;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
