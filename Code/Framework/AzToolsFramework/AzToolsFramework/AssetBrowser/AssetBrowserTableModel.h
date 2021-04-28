#pragma once
#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzCore/std/containers/vector.h>

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
        class AssetBrowserTableModel
            : public QSortFilterProxyModel
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTableModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserTableModel(QObject* parent = nullptr);

            QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
            QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
            //QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        public Q_SLOTS:
            void UpdateMap();

        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            bool filterAcceptsColumn(int source_column, const QModelIndex& /*source_parent*/) const override;

        private:
            int BuildMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;

        private:
            AZStd::fixed_unordered_set<int, 3, static_cast<int>(AssetBrowserEntry::Column::Count)> m_showColumn;
            QMap<int, QModelIndex> m_indexMap;
            QMap<QModelIndex, int> m_rowMap;
        };
    }
}
