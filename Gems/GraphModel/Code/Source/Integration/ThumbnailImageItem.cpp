/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QPainter>
#include <QWidget>

// GraphModel
#include <GraphModel/Integration/ThumbnailImageItem.h>

namespace GraphModelIntegration
{
    static const QSize IMAGE_MARGIN = QSize(10, 10);

    ThumbnailImageItem::ThumbnailImageItem(const QPixmap& image, QGraphicsItem* parent)
        : ThumbnailItem(parent)
        , m_pixmap(image)
    {
    }

    void ThumbnailImageItem::UpdateImage(const QPixmap& image)
    {
        m_pixmap = image;

        // Schedule a new paint request since we changed the pixmap
        update();
    }

    QSizeF ThumbnailImageItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
    {
        switch (which)
        {
        case Qt::MinimumSize:
        case Qt::PreferredSize:
            return m_pixmap.size() + IMAGE_MARGIN;
        case Qt::MaximumSize:
            return QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        default:
            break;
        }

        return constraint;
    }

    void ThumbnailImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        (void)option;
        (void)widget;

        // Draw the pixmap centered in our given frame
        QRectF frame(QPointF(0, 0), geometry().size());
        QPointF topLeft = frame.center() - (QPointF(m_pixmap.width(), m_pixmap.height()) / 2);
        painter->drawPixmap(topLeft, m_pixmap);
    }
}
