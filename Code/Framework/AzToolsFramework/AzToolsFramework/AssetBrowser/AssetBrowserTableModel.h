#pragma once
#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzCore/std/containers/vector.h>
//#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <QPointer>

AZ_PUSH_DISABLE_WARNING(
    4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QCollator>
#include <QSharedPointer>
#include <QSortFilterProxyModel>
#endif
AZ_POP_DISABLE_WARNING
namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;


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
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            QModelIndex parent(const QModelIndex& child) const override;
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        public Q_SLOTS:
            void UpdateMap();
        protected:
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const override;
            ////////////////////////////////////////////////////////////////////

        private:
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
            int BuildMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);
        private:
            QPointer<AssetBrowserFilterModel> m_filterModel;
            QMap<int, QModelIndex> m_indexMap;
            QMap<QModelIndex, int> m_rowMap;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
