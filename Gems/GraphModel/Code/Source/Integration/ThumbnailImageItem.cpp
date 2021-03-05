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
