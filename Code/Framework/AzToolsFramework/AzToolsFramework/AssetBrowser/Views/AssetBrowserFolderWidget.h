/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

class QAbstractItemModel;

namespace AzQtComponents
{
    class AssetFolderListView;
    class AssetFolderThumbnailView;
    class BreadCrumbs;
}

class QStackedWidget;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserFolderWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            explicit AssetBrowserFolderWidget(QWidget* parent = nullptr);
            ~AssetBrowserFolderWidget() override;

            void setModel(QAbstractItemModel* model);

            QString currentPath() const;
            void setCurrentPath(const QString& path);

        signals:
            void pathChanged(const QString& path);

        private:
            QStackedWidget* m_viewStack;
            AzQtComponents::BreadCrumbs* m_pathView;
            AzQtComponents::AssetFolderListView* m_listView;
            AzQtComponents::AssetFolderThumbnailView* m_thumbnailView;
        };
    }
}
