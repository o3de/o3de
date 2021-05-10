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
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(AssetBrowserTableModel, AZ::SystemAllocator, 0);
            explicit AssetBrowserTableModel(QObject* parent = nullptr);
            ~AssetBrowserTableModel();
            ////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            ////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;
            void setSourceModel(QAbstractItemModel* sourceModel) override;
            ////////////////////////////////////////////////////////////////////
            // QSortFilterProxyModel
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
            QModelIndex parent(const QModelIndex& child) const override;
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        public Q_SLOTS:
            void UpdateMap();
        protected:
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            //QVariant headerData(int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */) const override;
            ////////////////////////////////////////////////////////////////////

        private:
            AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
            int BuildMap(const QAbstractItemModel* model, const QModelIndex& parent = QModelIndex(), int row = 0);
        private:
            QMap<int, QModelIndex> m_indexMap;
            QMap<QModelIndex, int> m_rowMap;
        };

        class AssetBrowserTableFilterModel
            : public QSortFilterProxyModel
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT
        public:
            explicit AssetBrowserTableFilterModel(QObject* parent = nullptr);
            ~AssetBrowserTableFilterModel();

            void setSourceModel(QAbstractItemModel* sourceModel) override;
            // asset type filtering
            void SetFilter(FilterConstType filter);
            void FilterUpdatedSlotImmediate();

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void OnAssetBrowserComponentReady() override;

        Q_SIGNALS:
            void filterChanged();
            void entriesUpdated();

            //////////////////////////////////////////////////////////////////////////
            // QSortFilterProxyModel
        protected:
            bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
            bool filterAcceptsColumn(int source_column, const QModelIndex& /*source_parent*/) const override;
            bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
            //////////////////////////////////////////////////////////////////////////

        public Q_SLOTS:
            void filterUpdatedSlot();

        private:
            AZStd::fixed_unordered_set<int, 3, static_cast<int>(AssetBrowserEntry::Column::Count)> m_showColumn;
             bool m_alreadyRecomputingFilters = false;
            // asset source name match filter
             FilterConstType m_filter;
             AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
             QWeakPointer<const StringFilter> m_stringFilter;
             QWeakPointer<const CompositeFilter> m_assetTypeFilter;
             QCollator m_collator; // cache the collator as its somewhat expensive to constantly create and destroy one.
             AZ_POP_DISABLE_WARNING
             bool m_invalidateFilter = false;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
