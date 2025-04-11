/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/AssetFolderTableView.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: 'initializing': conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractItemDelegate>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSettings>
#include <QMenu>
AZ_POP_DISABLE_WARNING

namespace AzQtComponents
{
    AssetFolderTableView::AssetFolderTableView(QWidget* parent)
        : TableView(parent)
    {
        setSortingEnabled(true);
        setContextMenuPolicy(Qt::CustomContextMenu);
        setSelectionMode(ExtendedSelection);

        connect(this, &QAbstractItemView::clicked, this, &AssetFolderTableView::onClickedView);
    }

    void AssetFolderTableView::setRootIndex(const QModelIndex& index)
    {
        if (index != rootIndex())
        {
            QAbstractItemView::setRootIndex(index);
            emit tableRootIndexChanged(index);
        }
    }

    void AssetFolderTableView::SetShowSearchResultsMode(bool searchMode)
    {
        m_showSearchResultsMode = searchMode;
    }

    void AssetFolderTableView::mousePressEvent(QMouseEvent* event)
    {
        const auto p = event->pos();
        auto idx = indexAt(p);
        if (!idx.isValid() && selectionModel()->hasSelection())
        {
            selectionModel()->clear();
            emit rowDeselected();
        }
        else
        {
            TableView::mousePressEvent(event);
        }
    }

    void AssetFolderTableView::mouseDoubleClickEvent(QMouseEvent* event)
    {
        const auto p = event->pos();
        if (auto idx = indexAt(p); idx.isValid())
        {
            selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect | QItemSelectionModel::Rows);
            emit doubleClicked(idx);
        }
    }

    void AssetFolderTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        TableView::selectionChanged(selected, deselected);
        Q_EMIT selectionChangedSignal(selected, deselected);

    }

    void AssetFolderTableView::onClickedView(const QModelIndex& index)
    {
        // if we click on an item and selection wasn't changed, then reselect the current item so that it shows up in
        // any related previewers.
        if (selectionModel()->isSelected(index))
        {
            Q_EMIT selectionChangedSignal(selectionModel()->selection(), {});
        }
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_AssetFolderTableView.cpp"
