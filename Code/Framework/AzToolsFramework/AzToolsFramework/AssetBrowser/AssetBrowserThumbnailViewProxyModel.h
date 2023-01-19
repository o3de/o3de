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

            bool GetShowSearchResultsMode() const;
            void SetShowSearchResultsMode(bool searchMode);

        private:
            QPersistentModelIndex m_rootIndex;
            bool m_searchResultsMode;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
