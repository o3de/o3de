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
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserThumbnailViewProxyModel : public QAbstractProxyModel
        {
            Q_OBJECT

        public:
            explicit AssetBrowserThumbnailViewProxyModel(QObject* parent = nullptr);
            ~AssetBrowserThumbnailViewProxyModel() override;

            void SetSourceModelCurrentSelection(const QModelIndex& sourceIndex);

            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            QModelIndex parent(const QModelIndex& child) const override;

            QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
            QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;

            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        private:
            QPersistentModelIndex m_sourceSelectionIndex;
            AZStd::vector<int> m_sourceValidChildren;

            void RecomputeValidEntries();
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
