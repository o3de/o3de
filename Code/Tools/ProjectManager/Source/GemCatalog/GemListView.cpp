/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemItemDelegate.h>
#include <QStandardItemModel>

namespace O3DE::ProjectManager
{
    GemListView::GemListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent)
        : QListView(parent)
    {
        setObjectName("GemCatalogListView");
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        setModel(model);
        setSelectionModel(selectionModel);
        setItemDelegate(new GemItemDelegate(model, this));
    }
} // namespace O3DE::ProjectManager
