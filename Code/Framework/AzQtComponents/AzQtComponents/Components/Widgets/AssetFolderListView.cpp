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
#include <AzQtComponents/Components/Widgets/AssetFolderListView.h>

AZ_PUSH_DISABLE_WARNING(4244, "-Wunknown-warning-option")
#include <QHeaderView>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    AssetFolderListView::AssetFolderListView(QWidget* parent)
        : TableView(parent)
    {
        setRootIsDecorated(true);
        setUniformRowHeights(true);
        setSortingEnabled(true);
        header()->setStretchLastSection(false);
    }

    void AssetFolderListView::setModel(QAbstractItemModel* model)
    {
        QTreeView::setModel(model);
        for (int i = 0; i < header()->count(); ++i)
        {
            header()->setSectionResizeMode(i, i == 0 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
        }
    }
}

#include "Components/Widgets/moc_AssetFolderListView.cpp"
