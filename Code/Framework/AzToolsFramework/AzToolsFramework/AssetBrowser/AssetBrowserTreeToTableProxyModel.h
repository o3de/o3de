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
        class AssetBrowserTreeToTableProxyModel
            : public QAbstractProxyModel
        {
            Q_OBJECT

        public:
            AssetBrowserTreeToTableProxyModel(QObject* parent = nullptr);
            ~AssetBrowserTreeToTableProxyModel() override = default;

            void DisconnectSignals();
            void ConnectSignals();

            QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex parent(const QModelIndex& index) const override;
            QModelIndex index(int, int, const QModelIndex& parent = QModelIndex()) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& index = QModelIndex()) const override;

            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        private slots:
            void onSourceDataChanged(
                const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>());

        private:
            void FlattenTree();
            void TraverseTree(const QModelIndex& parent, int& currentRow);
            QModelIndex mapToTreeModel(const QModelIndex& proxyIndex) const;

            QHash<int, QPersistentModelIndex> flattenedData; // Maps proxy row to source index
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
