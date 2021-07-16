/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Qt
#include <QPixmap>

// GraphModel
#include <GraphModel/Integration/ThumbnailItem.h>

namespace GraphModelIntegration
{
    /**
     * Default image implementation of our ThumbnailItem class to draw a
     * simple QPixmap as the thumbnail.
     */
    class ThumbnailImageItem
        : public ThumbnailItem
    {
    public:
        AZ_RTTI(ThumbnailImageItem, "{DB2F488F-95CF-49BC-8DD4-806969A71A16}", ThumbnailItem);

        ThumbnailImageItem(const QPixmap& image, QGraphicsItem* parent = nullptr);

        void UpdateImage(const QPixmap& image);

        //! Override from QGraphicsLayoutItem
        QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const override;

        //! Override from QGraphicsItem
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

    protected:
        QPixmap m_pixmap;
    };
}
