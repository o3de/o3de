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
        class AssetBrowserExpandedTableViewProxyModel
            : public QIdentityProxyModel
        {
            Q_OBJECT

        public:
            explicit AssetBrowserExpandedTableViewProxyModel(QObject* parent = nullptr);
            ~AssetBrowserExpandedTableViewProxyModel() override;

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

            // Used to keep track of the root index on the view consuming this model, so that the model
            // can generate extra data such as whether an entry is on the top level.
            void SetRootIndex(const QModelIndex& index);
            const QModelIndex GetRootIndex() const;

            bool GetShowSearchResultsMode() const;
            void SetShowSearchResultsMode(bool searchMode);
            const AZStd::string ExtensionToType(AZStd::string_view str) const;

        private:
            QPersistentModelIndex m_rootIndex;
            bool m_searchResultsMode;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
