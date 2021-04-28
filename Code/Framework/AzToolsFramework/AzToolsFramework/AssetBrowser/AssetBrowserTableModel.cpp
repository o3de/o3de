#include "AssetBrowserTableModel.h"
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableModel::AssetBrowserTableModel(QObject* parent /* = nullptr */)
            : QSortFilterProxyModel(parent)
        {
            m_showColumn.insert(static_cast<int>(AssetBrowserEntry::Column::DisplayName));
            m_showColumn.insert(static_cast<int>(AssetBrowserEntry::Column::Path));
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
        QModelIndex AssetBrowserTableModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            Q_ASSERT(!sourceIndex.isValid() || sourceIndex.model() == sourceModel());
            if (!sourceIndex.isValid())
            {
                return QModelIndex();
            }
            return createIndex(m_rowMap[sourceIndex], sourceIndex.column(), sourceIndex.internalPointer());
        }
        //QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        //{

        //    //return parent.isValid() ? QModelIndex() : createIndex(row, column , m_indexMap[row].internalPointer());
        //    if (!parent.isValid())
        //    {
        //        QModelIndex();
        //    }
        //    return createIndex(row, column, m_indexMap[row].internalPointer());
        //}
        QVariant AssetBrowserTableModel::data(const QModelIndex& index, int role) const
        {
            //AZ_UNUSED(role);
            auto sourceIndex = mapToSource(index);
            if (!sourceIndex.isValid())
                return QVariant();

            AssetBrowserEntry* entry = GetAssetEntry(sourceIndex); // static_cast<AssetBrowserEntry*>(sourceIndex.internalPointer());
            if (entry == nullptr)
            {
                AZ_Assert(false, "ERROR - index internal pointer not pointing to an AssetEntry. Tree provided by the AssetBrowser invalid?");
                return Qt::PartiallyChecked;
            }

            return sourceIndex.data(role); //return entry->data(index.column());
            //return QVariant::fromValue(entry);

        }
        bool AssetBrowserTableModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            AZ_UNUSED(source_row);
            AZ_UNUSED(source_parent);
            return true;
        }
        bool AssetBrowserTableModel::filterAcceptsColumn(int source_column, const QModelIndex&) const
        {
            return m_showColumn.find(source_column) != m_showColumn.end();
        }
        int AssetBrowserTableModel::BuildMap(const QAbstractItemModel* model, const QModelIndex& parent, int row)
        {
            int rows = model ? model->rowCount(parent) : 0;
            for (int i = 0; i < rows; ++i)
            {
                auto index = model->index(i, 0, parent);

                m_rowMap[index] = row;
                m_indexMap[row] = index;
                row = row + 1;
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
            BuildMap(sourceModel());
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserTableModel.cpp"
