/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            m_showColumn.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::DisplayName));
            if (ed_useNewAssetBrowserTableView)
            {
                m_showColumn.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Path));
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
            return m_showColumn.find(source_column) != m_showColumn.end();
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
            auto compFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(m_filter);
            if (compFilter)
            {
                const auto& subFilters = compFilter->GetSubFilters();

                const auto compositeFilterIterator = AZStd::find_if(subFilters.cbegin(), subFilters.cend(), [subFilters](FilterConstType filter) -> bool
                {
                    const auto assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(filter);
                    return !assetTypeFilter.isNull();
                });

                if (compositeFilterIterator != subFilters.end())
                {
                    m_assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(*compositeFilterIterator);
                }

                const auto compStringFilterIter = AZStd::find_if(subFilters.cbegin(), subFilters.cend(), [](FilterConstType filter) -> bool
                {
                    //The real StringFilter is really a CompositeFilter with just one StringFilter in its subfilter list
                    //To know if it is actually a StringFilter we have to get that subfilter and check if it is a Stringfilter.
                    const auto stringCompositeFilter = qobject_cast<QSharedPointer<const CompositeFilter> >(filter);
                    bool isStringFilter = false;
                    if (stringCompositeFilter)
                    {
                        const auto& stringSubfilters = stringCompositeFilter->GetSubFilters();
                        auto canBeCasted = [](FilterConstType filt) -> bool
                        {
                            auto strFilter = qobject_cast<QSharedPointer<const StringFilter>>(filt);
                            return !strFilter.isNull();
                        };
                        const auto stringSubfliterConstIter = AZStd::find_if(stringSubfilters.cbegin(), stringSubfilters.cend(), canBeCasted);

                        //A Composite StringFilter will only have just one subfilter and nothing more.
                        if (stringSubfliterConstIter != stringSubfilters.end() && stringSubfilters.size() == 1)
                        {
                            isStringFilter = true;
                        }
                    }

                    return isStringFilter;
                });
                if (compStringFilterIter != subFilters.end())
                {
                    const auto compStringFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(*compStringFilterIter);

                    if (!compStringFilter->GetSubFilters().isEmpty() && compStringFilter->GetSubFilters()[0])
                    {
                        m_stringFilter = qobject_cast<QSharedPointer<const StringFilter>>(compStringFilter->GetSubFilters()[0]);
                    }
                }

            }
            invalidateFilter();
            Q_EMIT filterChanged();
            emit stringFilterPopulated(!m_stringFilter.isNull());
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
} // namespace AzToolsFramework// namespace AssetBrowser

#include "AssetBrowser/moc_AssetBrowserFilterModel.cpp"
