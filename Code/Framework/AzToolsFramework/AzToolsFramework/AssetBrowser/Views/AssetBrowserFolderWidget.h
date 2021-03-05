/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
