/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        setStyleSheet("background-color: #333333;");

        setModel(model);
        setSelectionModel(selectionModel);
        setItemDelegate(new GemItemDelegate(model, this));
    }
} // namespace O3DE::ProjectManager
