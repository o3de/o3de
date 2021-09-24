/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoListView.h>
#include <GemRepo/GemRepoItemDelegate.h>

namespace O3DE::ProjectManager
{
    GemRepoListView::GemRepoListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent)
        : QListView(parent)
    {
        setObjectName("gemRepoListView");
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        setModel(model);
        setSelectionModel(selectionModel);
        setItemDelegate(new GemRepoItemDelegate(model, this));
    }
} // namespace O3DE::ProjectManager
