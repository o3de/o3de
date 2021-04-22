#include "AssetBrowserTableModel.h"
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableModel::AssetBrowserTableModel(QObject* parent /* = nullptr */)
            : QSortFilterProxyModel(parent)
        {
        }
        QModelIndex AssetBrowserTableModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            AZ_UNUSED(proxyIndex);
            return QModelIndex();
        }
        QModelIndex AssetBrowserTableModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            AZ_UNUSED(sourceIndex);
            return QModelIndex();
        }
        QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        {
            AZ_UNUSED(row);
            AZ_UNUSED(column);
            AZ_UNUSED(parent);

            return QModelIndex();
        }
        QVariant AssetBrowserTableModel::data(const QModelIndex& index, int role) const
        {
            AZ_UNUSED(index);
            AZ_UNUSED(role);

            return QVariant();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserTableModel.cpp"
