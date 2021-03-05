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
