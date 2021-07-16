/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphModel/Integration/ThumbnailItem.h>

namespace GraphModelIntegration
{
    ThumbnailItem::ThumbnailItem(QGraphicsItem* parent)
        : QGraphicsLayoutItem()
        , QGraphicsItem(parent)
    {
        setGraphicsItem(this);
    }

    void ThumbnailItem::setGeometry(const QRectF &geom)
    {
        prepareGeometryChange();
        QGraphicsLayoutItem::setGeometry(geom);
        setPos(geom.topLeft());
    }

    QRectF ThumbnailItem::boundingRect() const
    {
        return QRectF(QPointF(0, 0), geometry().size());
    }
}
