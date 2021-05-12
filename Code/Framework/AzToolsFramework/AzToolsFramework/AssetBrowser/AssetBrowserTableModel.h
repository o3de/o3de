#pragma once
#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <QSortFilterProxyModel>
#include <QPointer>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
        class AssetBrowserEntry;

        class AssetBrowserTableModel
            : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTableModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserTableModel(QObject* parent = nullptr);
            ////////////////////////////////////////////////////////////////////
            // QSortFilterProxyModel
            void setSourceModel(QAbstractItemModel* sourceModel) override;
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
            QModelIndex parent(const QModelIndex& child) const override;
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        public Q_SLOTS:
            void UpdateTableModelMaps();
        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const override;
            ////////////////////////////////////////////////////////////////////

        private:
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
            int BuildTableModelMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);
        private:
            QPointer<AssetBrowserFilterModel> m_filterModel;
            QMap<int, QModelIndex> m_indexMap;
            QMap<QModelIndex, int> m_rowMap;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
