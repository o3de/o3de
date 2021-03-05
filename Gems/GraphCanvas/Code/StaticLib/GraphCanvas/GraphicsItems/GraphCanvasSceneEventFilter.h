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

#include <QGraphicsWidget>

namespace GraphCanvas
{
    namespace DataIdentifiers
    {
        constexpr int SceneEventFilter = 100;
    }

    class SceneEventFilter
        : public QGraphicsItem
    {
    public:
        AZ_CLASS_ALLOCATOR(SceneEventFilter, AZ::SystemAllocator, 0);

        SceneEventFilter(QGraphicsItem* parent)
            : QGraphicsItem(parent)
        {
            QVariant variantData = QVariant(true);
            setData(DataIdentifiers::SceneEventFilter, variantData);
        }

        QRectF boundingRect() const override
        {
            return QRectF();
        }

        void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override
        {
        }
    };
}
