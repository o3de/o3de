/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        using ShownColumnsSet = AZStd::fixed_unordered_set<int, 3, aznumeric_cast<int>(AssetBrowserEntry::Column::Count)>;

        class AssetBrowserFilterModel
            : public QSortFilterProxyModel
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT

        public:
            
            AZ_CLASS_ALLOCATOR(AssetBrowserFilterModel, AZ::SystemAllocator);
            explicit AssetBrowserFilterModel(QObject* parent = nullptr, bool isTableView = false);
            ~AssetBrowserFilterModel() override;

            // QSortFilterProxyModel
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

            //asset type filtering
            void SetFilter(FilterConstType filter);
            void FilterUpdatedSlotImmediate();

            const FilterConstType& GetFilter() const { return m_filter; }
            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;
            QSharedPointer<const StringFilter> GetStringFilter() const;

            void SetSortMode(const AssetBrowserEntry::AssetEntrySortMode sortMode);
            AssetBrowserEntry::AssetEntrySortMode GetSortMode() const;
            void SetSortOrder(const Qt::SortOrder sortOrder);
            Qt::SortOrder GetSortOrder() const;

            void SetSearchString(const QString& searchString);

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
            // Set for filtering columns
            // If the column is in the set the column is not filtered and is shown
            ShownColumnsSet m_shownColumns;
            bool m_alreadyRecomputingFilters = false;
            //Asset source name match filter
            FilterConstType m_filter;
            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
            QSharedPointer<const StringFilter> m_stringFilter;
            QCollator m_collator;  // cache the collator as its somewhat expensive to constantly create and destroy one.
            AZ_POP_DISABLE_WARNING
            bool m_invalidateFilter = false;
            
            bool m_isTableView{ false };
            AssetBrowserEntry::AssetEntrySortMode m_sortMode = AssetBrowserEntry::AssetEntrySortMode::Name;
            Qt::SortOrder m_sortOrder = Qt::DescendingOrder;
 
            QString m_searchString;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
