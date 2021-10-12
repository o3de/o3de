/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemItemDelegate.h>

#include <QMovie>

namespace O3DE::ProjectManager
{
    GemListView::GemListView(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QWidget* parent)
        : QListView(parent)
    {
        setObjectName("GemCatalogListView");
        setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        setModel(model);
        setSelectionModel(selectionModel);
        GemItemDelegate* itemDelegate = new GemItemDelegate(model, this);
        
        connect(itemDelegate, &GemItemDelegate::MovieStartedPlaying, [=](const QMovie* playingMovie)
            {
                // Force redraw when movie is playing so animation is smooth
                connect(playingMovie, &QMovie::frameChanged, this, [=]
                    {
                        this->viewport()->repaint();
                    });
            });

        setItemDelegate(itemDelegate);
    }
} // namespace O3DE::ProjectManager
