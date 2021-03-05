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
