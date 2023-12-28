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
        class AssetBrowserThumbnailViewProxyModel : public QIdentityProxyModel
        {
            Q_OBJECT

        public:
            explicit AssetBrowserThumbnailViewProxyModel(QObject* parent = nullptr);
            ~AssetBrowserThumbnailViewProxyModel() override;

            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

            // Used to keep track of the root index on the view consuming this model, so that the model
            // can generate extra data such as whether an entry is on the top level.
            void SetRootIndex(const QModelIndex& index);
            const QModelIndex GetRootIndex() const;

            bool GetShowSearchResultsMode() const;
            void SetShowSearchResultsMode(bool searchMode);

            void SetSearchString(const QString& searchString);

             //////////////////////////////////////////////////////////////////////////
            // QAbstractTableModel
            //////////////////////////////////////////////////////////////////////////
            Qt::DropActions supportedDropActions() const override;
            Qt::ItemFlags flags(const QModelIndex& index) const override;
            bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
            QMimeData* mimeData(const QModelIndexList& indexes) const override;
            QStringList mimeTypes() const override;
        private:
            QPersistentModelIndex m_rootIndex;
            bool m_searchResultsMode{ false };
            QString m_searchString;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
