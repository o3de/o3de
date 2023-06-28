/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractProxyModel>
#include <QSet>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        using TreeHash = QHash<QPersistentModelIndex, int>;
        using TableMap = QMap<int, QPersistentModelIndex>;
        using QPersistentModelIndexList = QList<QPersistentModelIndex>;

        using TableIterator = TableMap::iterator;
        using TreeIterator = TreeHash::iterator;
        using ConstTableIterator = TableMap::const_iterator;

        using TableType = TableMap::mapped_type;
        using TreeType = TreeHash::mapped_type;

        class IndexToMap
        {
        public:
            TableIterator TableLowerBound(const int& key);
            TableIterator TableUpperBound(const int& key);
            ConstTableIterator TableLowerBound(const int& key) const;
            ConstTableIterator TableConstBegin() const;
            ConstTableIterator TableConstEnd() const;
            TableIterator EraseFromTable(TableIterator it);
            TableIterator TableBegin();
            TableIterator TableEnd();
            bool Empty() const;
            bool TreeContains(TableType map) const;
            TreeType TreeToTable(TableType map) const;
            bool RemoveFromTree(TableType map);
            TreeIterator Insert(TableType tmap, TreeType row);
            void Clear();

        protected:
            TreeHash m_treeToTable;
            TableMap m_tableToTree;
        };

        class AssetBrowserTreeToTableProxyModel
            : public QAbstractProxyModel
        {
            Q_OBJECT

        public:
            explicit AssetBrowserTreeToTableProxyModel(QObject* parent = nullptr);
            ~AssetBrowserTreeToTableProxyModel() override = default;

            void setSourceModel(QAbstractItemModel* model) override;

            Qt::ItemFlags flags(const QModelIndex& index) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

            QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

            QModelIndex parent(const QModelIndex& index) const override;
            bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
            QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& index = QModelIndex()) const override;

            void ProcessParents();
            void RefreshMap();
            void DataChangedAllSiblings(const QModelIndex& parent);

        private slots:
            void RowsAboutToBeInserted(const QModelIndex& parent, int start, int end);
            void RowsInserted(const QModelIndex& parent, int start, int end);
            void RowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
            void RowsRemoved(const QModelIndex& parent, int start);
            void RowsMoved(const QModelIndex& srcParent, int srcStart, const QModelIndex& destParent, int destStart);
            void ModelReset();
            void LayoutAboutToBeChanged();
            void LayoutChanged();
            void DataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        private:
            void UpdateInternalIndices(int start, int offset);
            QModelIndex GetFirstDeepest(QAbstractItemModel* model, const QModelIndex& parent, int& count);

            IndexToMap m_map;
            int m_rowCount{ 0 };
            QPersistentModelIndexList m_parents;
            bool m_processing{ false };
            QPair<int, int> m_removePair;
            QPair<int, int> m_insertPair;
            QModelIndexList m_proxyIndices;
            QPersistentModelIndexList m_changePersistentIndexes;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
