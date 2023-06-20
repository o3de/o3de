/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(SceneEventFilter, AZ::SystemAllocator);

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
