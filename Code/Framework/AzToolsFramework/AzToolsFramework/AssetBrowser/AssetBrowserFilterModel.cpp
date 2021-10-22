/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzCore/Console/IConsole.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

#include <QSharedPointer>
#include <QTimer>
#include <QCollator>
AZ_POP_DISABLE_WARNING

AZ_CVAR(
    bool, ed_useNewAssetBrowserTableView, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Use the new AssetBrowser TableView for searching assets.");
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //AssetBrowserFilterModel
        AssetBrowserFilterModel::AssetBrowserFilterModel(QObject* parent)
            : QSortFilterProxyModel(parent)
        {
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName));
            if (ed_useNewAssetBrowserTableView)
            {
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Path));
            }
            m_collator.setNumericMode(true);
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }

        AssetBrowserFilterModel::~AssetBrowserFilterModel()
        {
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }

        void AssetBrowserFilterModel::SetFilter(FilterConstType filter)
        {
            connect(filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserFilterModel::filterUpdatedSlot);
            m_filter = filter;
            m_invalidateFilter = true;
            // asset browser entries are not guaranteed to have populated when the filter is set, delay filtering until they are
            bool isAssetBrowserComponentReady = false;
            AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
            if (isAssetBrowserComponentReady)
            {
                OnAssetBrowserComponentReady();
            }
        }

        void AssetBrowserFilterModel::OnAssetBrowserComponentReady()
        {
            if (m_invalidateFilter)
            {
                invalidateFilter();
                m_invalidateFilter = false;
            }
        }

        bool AssetBrowserFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            //get the source idx, if invalid early out
            QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
            if (!idx.isValid())
            {
                return false;
            }
            // no filter present, every entry is visible
            if (!m_filter)
            {
                return true;
            }

            //the entry is the internal pointer of the index
            auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

            // root should return true even if its not displayed in the treeview
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
            {
                return true;
            }
            return m_filter->Match(entry);
        }

        bool AssetBrowserFilterModel::filterAcceptsColumn(int source_column, const QModelIndex&) const
        {
            //if the column is in the set we want to show it
            return m_shownColumns.find(source_column) != m_shownColumns.end();
        }

        bool AssetBrowserFilterModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
        {
            if (source_left.column() == source_right.column())
            {
                QVariant leftData = sourceModel()->data(source_left, AssetBrowserModel::Roles::EntryRole);
                QVariant rightData = sourceModel()->data(source_right, AssetBrowserModel::Roles::EntryRole);
                if (leftData.canConvert<const AssetBrowserEntry*>() && rightData.canConvert<const AssetBrowserEntry*>())
                {
                    auto leftEntry = qvariant_cast<const AssetBrowserEntry*>(leftData);
                    auto rightEntry = qvariant_cast<const AssetBrowserEntry*>(rightData);

                    // folders should always come first
                    if (azrtti_istypeof<const FolderAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const SourceAssetBrowserEntry*>(rightEntry))
                    {
                        return false;
                    }
                    if (azrtti_istypeof<const SourceAssetBrowserEntry*>(leftEntry) && azrtti_istypeof<const FolderAssetBrowserEntry*>(rightEntry))
                    {
                        return true;
                    }

                    // if both entries are of same type, sort alphabetically
                    return m_collator.compare(leftEntry->GetDisplayName(), rightEntry->GetDisplayName()) > 0;
                }
            }
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }

        void AssetBrowserFilterModel::FilterUpdatedSlotImmediate()
        {
            const auto compFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(m_filter);
            if (compFilter)
            {
                const auto& subFilters = compFilter->GetSubFilters();
                const auto& compFilterIter = AZStd::find_if(subFilters.cbegin(), subFilters.cend(),
                    [](FilterConstType filter) -> bool
                    {
                        const auto assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(filter);
                        return !assetTypeFilter.isNull();
                    });

                if (compFilterIter != subFilters.end())
                {
                    m_assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(*compFilterIter);
                }

                const auto& compositeStringFilterIter = AZStd::find_if(subFilters.cbegin(), subFilters.cend(),
                    [](FilterConstType filter) -> bool
                    {
                        // The real StringFilter is really a CompositeFilter with just one StringFilter in its subfilter list
                        // To know if it is actually a StringFilter we have to get that subfilter and check if it is a Stringfilter.
                        const auto& stringCompositeFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(filter);
                        if (stringCompositeFilter)
                        {
                            //Once we have the main composite filter we can now obtain its subfilters and check if
                            //it has a StringFilter
                            const auto& stringSubfilters = stringCompositeFilter->GetSubFilters();
                            auto canBeCasted = [](FilterConstType filt) -> bool
                            {
                                const auto& strFilter = qobject_cast<QSharedPointer<const StringFilter>>(filt);
                                return !strFilter.isNull();
                            };

                            const auto& stringSubFilterConstIt  =
                                AZStd::find_if(stringSubfilters.cbegin(), stringSubfilters.cend(), canBeCasted);

                            // A Composite StringFilter will only have just one subfilter (the StringFilter) and nothing more.
                            return stringSubFilterConstIt != stringSubfilters.end() && stringSubfilters.size() == 1;
                        }
                        return false;
                    });

                if (compositeStringFilterIter != subFilters.end())
                {
                    const auto& compStringFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(*compositeStringFilterIter);

                    if (!compStringFilter->GetSubFilters().isEmpty())
                    {
                        const auto& stringFilter = compStringFilter->GetSubFilters()[0];
                        AZ_Assert(
                            stringFilter,
                            "AssetBrowserFilterModel - String Filter is not a valid Composite Filter");
                        m_stringFilter = qobject_cast<QSharedPointer<const StringFilter>>(stringFilter);
                    }
                }
            }
            invalidateFilter();
            
            Q_EMIT filterChanged();
        }

        void AssetBrowserFilterModel::filterUpdatedSlot()
        {
            if (!m_alreadyRecomputingFilters)
            {
                m_alreadyRecomputingFilters = true;
                // de-bounce it, since we may get many filter updates all at once.
                QTimer::singleShot(0, this, [this]()
                {
                    m_alreadyRecomputingFilters = false;
                    FilterUpdatedSlotImmediate();
                }
                );
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserFilterModel.cpp"
