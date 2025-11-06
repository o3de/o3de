/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ParticleGraphicsScence.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsView>

namespace OpenParticleSystemEditor
{
    const qreal PARTICLE_SCENE_X = -2000;
    const qreal PARTICLE_SCENE_Y = -2000;
    const qreal PARTICLE_SCENE_W = 4000;
    const qreal PARTICLE_SCENE_H = 4000;
    const int GRID_WIDTH = 50;
    const int GRID_THICKNESS = 1;
    const QColor GRID_COLOR = QColor(34, 34, 34);

    ParticleGraphicsScence::ParticleGraphicsScence()
        : m_bPressed(false)
    {
        setItemIndexMethod(QGraphicsScene::NoIndex);
        setMinimumRenderSize(2.0f);
        setSceneRect(PARTICLE_SCENE_X, PARTICLE_SCENE_Y, PARTICLE_SCENE_W, PARTICLE_SCENE_H);
    }

    ParticleGraphicsScence::~ParticleGraphicsScence()
    {
    }

    void ParticleGraphicsScence::mousePressEvent(QGraphicsSceneMouseEvent* event)
    {
        QGraphicsScene::mousePressEvent(event);

        if (event->button() == Qt::LeftButton)
        {
            m_pItemSelected = nullptr;
            for (QGraphicsItem* item : items(event->scenePos()))
            {
                if (item->type() == QGraphicsProxyWidget::Type)
                {
                    m_bPressed = true;
                    m_Pressed = event->scenePos();
                    m_pItemSelected = item;

                    QGraphicsProxyWidget* pProxyWidget = qgraphicsitem_cast<QGraphicsProxyWidget*>(item);
                    m_ItemPos = pProxyWidget->scenePos();
                    break;
                }
            }
        }
    }

    void ParticleGraphicsScence::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
    {
        QGraphicsScene::mouseMoveEvent(event);

        if (m_pItemSelected != nullptr)
        {
            if (m_bPressed)
            {
                m_Offset = event->scenePos() - m_Pressed;
                m_pItemSelected->setPos(m_ItemPos + m_Offset);
            }
        }
    }

    void ParticleGraphicsScence::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
    {
        QGraphicsScene::mouseMoveEvent(event);

        m_bPressed = false;
        if (m_pItemSelected != nullptr)
        {
            m_pItemSelected = nullptr;
        }
    }

    void ParticleGraphicsScence::drawBackground(QPainter* painter, const QRectF& rect)
    {
        QPen pen;
        pen.setColor(GRID_COLOR);
        pen.setWidth(GRID_THICKNESS);
        painter->setPen(pen);
        qreal left = rect.left();
        qreal right = rect.right();
        qreal top = rect.top();
        qreal bottom = rect.bottom();

        left = (left / GRID_WIDTH) * GRID_WIDTH;
        right = (right / GRID_WIDTH) * GRID_WIDTH;
        top = (top / GRID_WIDTH) * GRID_WIDTH;
        bottom = (bottom / GRID_WIDTH) * GRID_WIDTH;

        for (int i = 0; i >= top; i -= GRID_WIDTH)
        {
            painter->drawLine(int(left), i, int(right), i);
        }

        for (int i = 0; i <= bottom; i += GRID_WIDTH)
        {
            painter->drawLine(int(left), i, int(right), i);
        }

        for (int i = 0; i <= right; i += GRID_WIDTH)
        {
            painter->drawLine(i, int(top), i, int(bottom));
        }

        for (int i = 0; i >= left; i -= GRID_WIDTH)
        {
            painter->drawLine(i, int(top), i, int(bottom));
        }
    }
} // namespace OpenParticleSystemEditor
