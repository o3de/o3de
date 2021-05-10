#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")

#include <AssetBrowser/AssetBrowserTableModel.h>
#include <AssetBrowser/AssetBrowserFilterModel.h>

#include <QCollator>
#include <QSharedPointer>
#include <QTimer>
AZ_POP_DISABLE_WARNING
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableModel::AssetBrowserTableModel(QObject* parent /* = nullptr */)
            : QSortFilterProxyModel(parent)
        {
            sort(0);
            setDynamicSortFilter(false);
            setRecursiveFilteringEnabled(true);
            AssetBrowserComponentNotificationBus::Handler::BusConnect();
        }
        AssetBrowserTableModel::~AssetBrowserTableModel()
        {
            AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }
        void AssetBrowserTableModel::OnAssetBrowserComponentReady()
        {
            //BuildMap(sourceModel());
        }
        void AssetBrowserTableModel::setSourceModel(QAbstractItemModel* sourceModel)
        {
            m_filterModel = qobject_cast<AssetBrowserFilterModel*>(sourceModel);
            AZ_Assert(m_filterModel, "Expecting AssetBrowserFilterModel");
            QSortFilterProxyModel::setSourceModel(sourceModel);
        }

        QModelIndex AssetBrowserTableModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            Q_ASSERT(!proxyIndex.isValid() || proxyIndex.model() == this);
            if (!proxyIndex.isValid())
            {
                return QModelIndex();
            }
            return m_indexMap[proxyIndex.row()];
        }
        QModelIndex AssetBrowserTableModel::parent(const QModelIndex& child) const
        {
            AZ_UNUSED(child);
            return QModelIndex();
        }
        QModelIndex AssetBrowserTableModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            Q_ASSERT(!sourceIndex.isValid() || sourceIndex.model() == sourceModel());
            if (!sourceIndex.isValid())
            {
                return QModelIndex();
            }
            return createIndex(m_rowMap[sourceIndex], sourceIndex.column(), sourceIndex.internalPointer());
        }

        bool AssetBrowserTableModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            AZ_UNUSED(source_row);
            AZ_UNUSED(source_parent);
            // no filter present, every entry is not visible
            if (!m_filterModel->GetFilter())
            {
                return false;
            }
            return true;
        }

        QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        {
            /*AZ_UNUSED(row);
            AZ_UNUSED(column);*/
            return parent.isValid() ? QModelIndex() : createIndex(row, column, m_indexMap[row].internalPointer());
        }

        QVariant AssetBrowserTableModel::data(const QModelIndex& index, int role) const
        {
            //AZ_UNUSED(role);
            auto sourceIndex = mapToSource(index);
            if (!sourceIndex.isValid())
                return QVariant();

            AssetBrowserEntry* entry = GetAssetEntry(sourceIndex); // static_cast<AssetBrowserEntry*>(sourceIndex.internalPointer());
            if (entry == nullptr)
            {
                AZ_Assert(
                    false, "ERROR - index internal pointer not pointing to an AssetEntry. Tree provided by the AssetBrowser invalid?");
                return Qt::PartiallyChecked;
            }

            return sourceIndex.data(role); // return entry->data(index.column());
            //return QVariant::fromValue(entry);
            //AZ_UNUSED(role);
            //if (index.isValid())
            //{
            //    ////if (role == AssetBrowserModel::EntryRole)
            //    //{
            //    QModelIndex modelIndex = mapFromSource(index);
            //    auto assetEntry = static_cast<const AssetBrowserEntry*>(index.internalPointer());
            //    return QVariant::fromValue(assetEntry);
            //}
            //return QVariant(); // AzToolsFramework::AssetBrowser::AssetBrowserModel::data(index, role);

        }


        int AssetBrowserTableModel::rowCount(const QModelIndex& parent) const
        {
            return !parent.isValid() ? m_rowMap.size() : 0;
        }

        int AssetBrowserTableModel::BuildMap(const QAbstractItemModel* model, const QModelIndex& parent, int row)
        {
            //int rows = model ? model->rowCount(parent) : 0;
            //for (int i = 0; i < rows; ++i)
            //{
            //    auto index = model->index(i, 0, parent);
            //    //if (!model->hasChildren(index))
            //    //{
            //        beginInsertRows(parent, row, row);
            //        m_rowMap[index] = row;
            //        m_indexMap[row] = index;
            //        endInsertRows();
            //        Q_EMIT dataChanged(parent, parent);
            //        row = row + 1;
            //    //}
            //    if (model->hasChildren(index))
            //    {
            //        row = BuildMap(model, index, row);
            //    }
            //}
            //return row;
            int rows = model ? model->rowCount(parent) : 0;
            for (int i = 0; i < rows; ++i)
            {
                auto index = model->index(i, 0, parent);
                if (model->hasChildren(index) == false)
                {
                    beginInsertRows(parent, row, row);
                    m_rowMap[index] = row;
                    m_indexMap[row] = index;
                    endInsertRows();
                    Q_EMIT dataChanged(parent, parent);
                    row = row + 1;
                }

                if (model->hasChildren(index))
                {
                    row = BuildMap(model, index, row);
                }
            }
            return row;
        }
        AssetBrowserEntry* AssetBrowserTableModel::GetAssetEntry(QModelIndex index) const
        {
            if (index.isValid())
            {
                return static_cast<AssetBrowserEntry*>(index.internalPointer());
            }
            else
            {
                AZ_Error("AssetBrowser", false, "Invalid Source Index provided to GetAssetEntry.");
                return nullptr;
            }
        }
        void AssetBrowserTableModel::UpdateMap()
        {
            //m_indexMap.clear();
            //m_rowMap.clear();

            if (m_indexMap.size() > 0)
            {
                //beginRemoveRows(m_indexMap.first().parent(), m_indexMap.first().row(), m_indexMap.last().row()); 
                for (const auto& key : m_indexMap.keys())
                {
                    beginRemoveRows(m_indexMap[key], m_indexMap[key].row(), m_indexMap[key].row());
                    m_rowMap.remove(m_indexMap[key]);
                    m_indexMap.remove(key);
                    endRemoveRows();
                }
                //endRemoveRows();
            }

            BuildMap(sourceModel());
        }

        //---------------------------------------AssetBrowserTableFilterModel--------------------------------------------
        //AssetBrowserTableFilterModel::AssetBrowserTableFilterModel(QObject* parent)
        //    : QSortFilterProxyModel(parent)
        //{
        //    m_showColumn.insert(static_cast<int>(AssetBrowserEntry::Column::DisplayName));
        //    m_showColumn.insert(static_cast<int>(AssetBrowserEntry::Column::Path));
        //    AssetBrowserComponentNotificationBus::Handler::BusConnect();
        //}

        //AssetBrowserTableFilterModel::~AssetBrowserTableFilterModel()
        //{
        //    AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        //}

        //void AssetBrowserTableFilterModel::setSourceModel(QAbstractItemModel* sourceModel)
        //{
        //    QSortFilterProxyModel::setSourceModel(sourceModel);
        //}

        //void AssetBrowserTableFilterModel::SetFilter(FilterConstType filter)
        //{
        //    connect(filter.data(), &AssetBrowserEntryFilter::updatedSignal, this, &AssetBrowserTableFilterModel::filterUpdatedSlot);
        //    m_filter = filter;
        //    m_invalidateFilter = true;
        //    // asset browser entries are not guaranteed to have populated when the filter is set, delay filtering until they are
        //    bool isAssetBrowserComponentReady = false;
        //    AssetBrowserComponentRequestBus::BroadcastResult(isAssetBrowserComponentReady, &AssetBrowserComponentRequests::AreEntriesReady);
        //    if (isAssetBrowserComponentReady)
        //    {
        //        OnAssetBrowserComponentReady();
        //    }
        //}

        //void AssetBrowserTableFilterModel::FilterUpdatedSlotImmediate()
        //{
        //    auto compFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(m_filter);
        //    if (compFilter)
        //    {
        //        auto& subFilters = compFilter->GetSubFilters();
        //        auto it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool {
        //            auto assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(filter);
        //            return !assetTypeFilter.isNull();
        //        });
        //        if (it != subFilters.end())
        //        {
        //            m_assetTypeFilter = qobject_cast<QSharedPointer<const CompositeFilter>>(*it);
        //        }
        //        it = AZStd::find_if(subFilters.begin(), subFilters.end(), [subFilters](FilterConstType filter) -> bool {
        //            auto stringFilter = qobject_cast<QSharedPointer<const StringFilter>>(filter);
        //            return !stringFilter.isNull();
        //        });
        //        if (it != subFilters.end())
        //        {
        //            m_stringFilter = qobject_cast<QSharedPointer<const StringFilter>>(*it);
        //        }
        //    }
        //    invalidateFilter();
        //    Q_EMIT filterChanged();
        //}

        //void AssetBrowserTableFilterModel::OnAssetBrowserComponentReady()
        //{
        //    if (m_invalidateFilter)
        //    {
        //        invalidateFilter();
        //        m_invalidateFilter = false;
        //    }
        //    Q_EMIT entriesUpdated();
        //}

        //bool AssetBrowserTableFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        //{
        //    AZ_UNUSED(source_row);
        //    AZ_UNUSED(source_parent);
        //    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
        //    if (!idx.isValid())
        //    {
        //        return false;
        //    }
        //    // no filter present, every entry is visible
        //    if (!m_filter)
        //    {
        //        return true;
        //    }

        //    //// the entry is the internal pointer of the index
        //    //auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

        //    //if (entry)
        //    //{
        //    //    // root should return true even if its not displayed in the treeview
        //    //    if (entry && entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Root)
        //    //    {
        //    //        return true;
        //    //    }
        //    //    return m_filter->Match(entry);
        //    //}
        //    return true;
        //}

        //bool AssetBrowserTableFilterModel::filterAcceptsColumn(int source_column, const QModelIndex&) const
        //{
        //    return m_showColumn.find(source_column) != m_showColumn.end();
        //}

        //bool AssetBrowserTableFilterModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
        //{
        //    if (source_left.column() == source_right.column())
        //    {
        //        QVariant leftData = sourceModel()->data(source_left, AssetBrowserModel::Roles::EntryRole);
        //        QVariant rightData = sourceModel()->data(source_right, AssetBrowserModel::Roles::EntryRole);
        //        if (leftData.canConvert<const AssetBrowserEntry*>() && rightData.canConvert<const AssetBrowserEntry*>())
        //        {
        //            auto leftEntry = qvariant_cast<const AssetBrowserEntry*>(leftData);
        //            auto rightEntry = qvariant_cast<const AssetBrowserEntry*>(rightData);

        //            // folders should always come first
        //            if (azrtti_istypeof<const FolderAssetBrowserEntry*>(leftEntry) &&
        //                azrtti_istypeof<const SourceAssetBrowserEntry*>(rightEntry))
        //            {
        //                return false;
        //            }
        //            if (azrtti_istypeof<const SourceAssetBrowserEntry*>(leftEntry) &&
        //                azrtti_istypeof<const FolderAssetBrowserEntry*>(rightEntry))
        //            {
        //                return true;
        //            }

        //            // if both entries are of same type, sort alphabetically
        //            return m_collator.compare(leftEntry->GetDisplayName(), rightEntry->GetDisplayName()) > 0;
        //        }
        //    }
        //    return QSortFilterProxyModel::lessThan(source_left, source_right);
        //}

        //void AssetBrowserTableFilterModel::filterUpdatedSlot()
        //{
        //    if (!m_alreadyRecomputingFilters)
        //    {
        //        m_alreadyRecomputingFilters = true;
        //        // de-bounce it, since we may get many filter updates all at once.
        //        QTimer::singleShot(0, this, [this]() {
        //            m_alreadyRecomputingFilters = false;
        //            FilterUpdatedSlotImmediate();
        //        });
        //    }
        //}

    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserTableModel.cpp"
