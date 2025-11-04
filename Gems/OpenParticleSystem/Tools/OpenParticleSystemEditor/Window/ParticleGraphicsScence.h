/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <QGraphicsScene>

namespace OpenParticleSystemEditor
{
    class ParticleGraphicsScence : public QGraphicsScene
    {
    public:
        ParticleGraphicsScence();
        virtual ~ParticleGraphicsScence();

    protected:
        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        void drawBackground(QPainter* painter, const QRectF& rect) override;

    private:
        QGraphicsItem* m_pItemSelected;
        QPointF m_Offset;
        QPointF m_Pressed;
        QPointF m_ItemPos;
        bool m_bPressed;
    };
} // namespace OpenParticleSystemEditor
