/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/AssetFolderExpandedTableView.h>

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
    AssetFolderExpandedTableView::AssetFolderExpandedTableView(QWidget* parent)
        : TableView(parent)
    {
        setSortingEnabled(true);
    }

    AssetFolderExpandedTableView::~AssetFolderExpandedTableView() = default;

    void AssetFolderExpandedTableView::setRootIndex(const QModelIndex& index)
    {
        if (index != rootIndex())
        {
            QAbstractItemView::setRootIndex(index);
            emit rootIndexChanged(index);
        }
    }


    void AssetFolderExpandedTableView::SetShowSearchResultsMode(bool searchMode)
    {
        m_showSearchResultsMode = searchMode;
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_AssetFolderExpandedTableView.cpp"
