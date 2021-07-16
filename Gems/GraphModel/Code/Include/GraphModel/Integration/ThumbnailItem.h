/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/RTTI/RTTI.h>

// Qt
#include <QGraphicsItem>
#include <QGraphicsLayoutItem>

namespace GraphModelIntegration
{
    /**
     * Base layout item class for embedding thumbnails inside a Node. The paint()
     * method can be overriden to implement any custom rendering desired.
     */
    class ThumbnailItem
        : public QGraphicsLayoutItem
        , public QGraphicsItem
    {
    public:
        AZ_RTTI(ThumbnailItem, "{4248ADDE-4DFF-4A02-A8FD-B992E3CFF94B}");

        ThumbnailItem(QGraphicsItem* parent = nullptr);

        //! Override from QGraphicsLayoutItem
        void setGeometry(const QRectF &geom) override;

        //! Override from QGraphicsItem
        QRectF boundingRect() const override;
    };
}
