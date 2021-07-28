/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
