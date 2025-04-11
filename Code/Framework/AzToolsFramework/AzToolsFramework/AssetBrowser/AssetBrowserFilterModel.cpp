/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#include <AzCore/Console/IConsole.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeToTableProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

#include <QCollator>
#include <QSharedPointer>
#include <QTimer>
AZ_POP_DISABLE_WARNING

AZ_CVAR(
    bool, ed_useNewAssetBrowserListView, true, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Use the new AssetBrowser ListView for searching assets.");
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //AssetBrowserFilterModel
        AssetBrowserFilterModel::AssetBrowserFilterModel(QObject* parent, bool isTableView)
            : QSortFilterProxyModel(parent)
            , m_isTableView(isTableView)
        {
            setDynamicSortFilter(true);
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName));
            if (ed_useNewAssetBrowserListView)
            {
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Path));
            }
            if (isTableView)
            {
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Type));
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::DiskSize));
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Vertices));
                m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::ApproxSize));
                // The below isn't used at present but will be needed in future
                // m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::SourceControlStatus));
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
            if (m_filter.data())
            {
                disconnect(m_filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserFilterModel::filterUpdatedSlot);
            }
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
                beginResetModel();
                m_invalidateFilter = false;
                endResetModel();
            }
        }

        QSharedPointer<const StringFilter> AssetBrowserFilterModel::GetStringFilter() const
        {
            return m_stringFilter;
        }

        QVariant AssetBrowserFilterModel::data(const QModelIndex& index, int role) const
        {
            if (!index.isValid())
            {
                return QVariant();
            }
            QModelIndex sourceIndex = mapToSource(index);

            if (!sourceIndex.isValid())
            {
                return QVariant(); // the view may be in a state of repopulating.
            }

            auto assetBrowserEntry = sourceIndex.data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
            AZ_Assert(assetBrowserEntry, "Couldn't fetch asset entry for the given index.");
            if (!assetBrowserEntry)
            {
                return tr(" No Data ");
            }

            if (role == Qt::DisplayRole)
            {
                if (index.column() == aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
                {
                    return AssetBrowserViewUtils::GetAssetBrowserEntryNameWithHighlighting(assetBrowserEntry, m_searchString);
                }
                
            }
            else if (role == static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsExactMatch))
            {
                auto entry = static_cast<AssetBrowserEntry*>(mapToSource(index).internalPointer());
                return !m_filter || m_filter->MatchWithoutPropagation(entry);
            }

            return QSortFilterProxyModel::data(index, role);
        }

        void AssetBrowserFilterModel::SetSearchString(const QString& searchString)
        {
            m_searchString = searchString;
        }

        bool AssetBrowserFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            //get the source idx, if invalid early out
            QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
            if (m_isTableView && qobject_cast<AssetBrowserTreeToTableProxyModel*>(sourceModel()))
            {
                idx = static_cast<AssetBrowserTreeToTableProxyModel*>(sourceModel())->mapToSource(idx);
            }

            if (!idx.isValid())
            {
                return false;
            }

            //the entry is the internal pointer of the index
            auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

            // root should return true even if its not displayed in the treeview
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
            {
                return true;
            }

            return !m_filter || m_filter->Match(entry);
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

                    return leftEntry->lessThan(rightEntry, m_sortMode, m_collator);
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
            // Note that because the data we are filtering over is massive (all assets) its way faster
            // to reset the model than it is to try to incrementally apply filters here, which can cause many more
            // messages like "row added / row removed" to be sent to the view.
            beginResetModel();
            endResetModel();

            Q_EMIT filterChanged();
        }

        void AssetBrowserFilterModel::filterUpdatedSlot()
        {
            if (!m_alreadyRecomputingFilters)
            {
                m_alreadyRecomputingFilters = true;
                // de-bounce it, since we may get many filter updates all at once.
                // do not use a 0 here, as this puts the message directly in the message queue, and will interleave
                // it with keypress events / referesh events, etc.
                QTimer::singleShot(20, this, [this]()
                {
                    m_alreadyRecomputingFilters = false;
                    FilterUpdatedSlotImmediate();
                }
                );
            }
        }

        void AssetBrowserFilterModel::SetSortMode(const AssetBrowserEntry::AssetEntrySortMode sortMode)
        {
            m_sortMode = sortMode;
        }

        AssetBrowserEntry::AssetEntrySortMode AssetBrowserFilterModel::GetSortMode() const
        {
            return m_sortMode;
        }

        void AssetBrowserFilterModel::SetSortOrder(const Qt::SortOrder sortOrder)
        {
            m_sortOrder = sortOrder;
        }

        Qt::SortOrder AssetBrowserFilterModel::GetSortOrder() const
        {
            return m_sortOrder;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserFilterModel.cpp"
