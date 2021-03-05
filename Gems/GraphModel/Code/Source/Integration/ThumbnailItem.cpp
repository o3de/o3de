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
