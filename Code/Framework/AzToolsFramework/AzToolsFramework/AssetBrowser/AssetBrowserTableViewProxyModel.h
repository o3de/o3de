/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QIdentityProxyModel>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserTableViewProxyModel
            : public QIdentityProxyModel
        {
            Q_OBJECT

        public:
            explicit AssetBrowserTableViewProxyModel(QObject* parent = nullptr);
            ~AssetBrowserTableViewProxyModel() override;

            enum
            {
                Name,
                Type,
                DiskSize,
                Vertices,
                ApproxSize,
                ColumnCount
            };
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
            int columnCount(const QModelIndex& parent = QModelIndex()) const override;
            bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
            bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

            // Used to keep track of the root index on the view consuming this model, so that the model
            // can generate extra data such as whether an entry is on the top level.
            void SetRootIndex(const QModelIndex& index);
            const QModelIndex GetRootIndex() const;

            bool GetShowSearchResultsMode() const;
            void SetShowSearchResultsMode(bool searchMode);

            void SetSearchString(const QString& searchString);
        private:
            QPersistentModelIndex m_rootIndex;
            bool m_searchResultsMode{ false };
            QString m_searchString;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
