/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SliceFavorites_precompiled.h"

#include "SliceFavoritesTreeView.h"
#include "FavoriteDataModel.h"

#include <AzCore/std/sort.h>
#include <AzCore/std/algorithm.h>

#include <QDrag>
#include <QPainter>

namespace SliceFavorites
{
    SliceFavoritesTreeView::SliceFavoritesTreeView(QWidget* pParent)
        : QTreeView(pParent)
    {
    }

    SliceFavoritesTreeView::~SliceFavoritesTreeView()
    {
    }

    void SliceFavoritesTreeView::startDrag(Qt::DropActions supportedActions)
    {
        const QModelIndexList indexListSorted = SortIndicesByHierarchicalLocation(selectionModel()->selectedIndexes());

        QMimeData* mimeData = model()->mimeData(indexListSorted);
        if (mimeData)
        {
            //initiate drag/drop for the item
            QDrag* drag = new QDrag(this);
            drag->setPixmap(QPixmap::fromImage(createDragImage(indexListSorted)));
            drag->setMimeData(mimeData);
            Qt::DropAction defDropAction = Qt::IgnoreAction;
            if (defaultDropAction() != Qt::IgnoreAction && (supportedActions & defaultDropAction()))
            {
                defDropAction = defaultDropAction();
            }
            else if (supportedActions & Qt::CopyAction && dragDropMode() != QAbstractItemView::InternalMove)
            {
                defDropAction = Qt::CopyAction;
            }
            drag->exec(supportedActions, defDropAction);
        }
    }
    
    QModelIndexList SliceFavoritesTreeView::SortIndicesByHierarchicalLocation(const QModelIndexList& indexList) const
    {
        //sort by parent depth and order in hierarchy for proper drag image and drop order
        QModelIndexList indexListSorted = indexList;
        AZStd::unordered_map<const void*, AZStd::list<int>> hierarchy;
        for (auto index : indexListSorted)
        {
            ConstructHierarchyData(index, hierarchy[index.internalPointer()]);
        }
        AZStd::sort(indexListSorted.begin(), indexListSorted.end(), [&hierarchy](const QModelIndex& index1, const QModelIndex& index2) 
        {
            const auto& hierarchyIndex1 = hierarchy[index1.internalPointer()];
            const auto& hierarchyIndex2 = hierarchy[index2.internalPointer()];
            return AZStd::lexicographical_compare(hierarchyIndex1.begin(), hierarchyIndex1.end(), hierarchyIndex2.begin(), hierarchyIndex2.end());
        });

        return indexListSorted;
    }

    void SliceFavoritesTreeView::ConstructHierarchyData(const QModelIndex& index, AZStd::list<int>& hierarchy) const
    {
        if (index.isValid())
        {
            hierarchy.push_front(index.row());
            ConstructHierarchyData(index.parent(), hierarchy);
        }
    }

    QImage SliceFavoritesTreeView::createDragImage(const QModelIndexList& indexList) const
    {
        //generate a drag image of the item icon and text, normally done internally, and inaccessible 
        QRect rect(0, 0, 0, 0);
        for (auto index : indexList)
        {
            QRect itemRect = visualRect(index);
            rect.setHeight(rect.height() + itemRect.height());
            rect.setWidth(AZStd::GetMax(rect.width(), itemRect.width()));
        }

        QImage dragImage(rect.size(), QImage::Format_ARGB32_Premultiplied);

        QPainter dragPainter(&dragImage);
        dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
        dragPainter.fillRect(dragImage.rect(), Qt::transparent);
        dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        dragPainter.setOpacity(0.35f);
        dragPainter.fillRect(rect, QColor("#222222"));
        dragPainter.setOpacity(1.0f);

        int imageY = 0;
        for (auto index : indexList)
        {
            QRect itemRect = visualRect(index);
//             dragPainter.drawPixmap(QPoint(0, imageY),
//                 model()->data(index, Qt::DecorationRole).value<QIcon>().pixmap(QSize(16, 16)));
            dragPainter.setPen(
                model()->data(index, Qt::ForegroundRole).value<QBrush>().color());
            dragPainter.setFont(
                font());
            dragPainter.drawText(QRect(20, imageY, rect.width() - 20, rect.height()),
                model()->data(index, Qt::DisplayRole).value<QString>());
            imageY += itemRect.height();
        }

        dragPainter.end();
        return dragImage;
    }
}
#include <Source/moc_SliceFavoritesTreeView.cpp>

