/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientWidget.h"

#include <QGradientStops>
#include <QPixmap>

namespace OpenParticleSystemEditor
{
    GradientWidget::GradientWidget(QWidget* parent)
        : QWidget(parent)
    {
    }

    GradientWidget::~GradientWidget()
    {
        while (m_gradients.count() > 0)
        {
            m_gradients.takeAt(0);
        }
    }

    void GradientWidget::mousePressEvent(QMouseEvent* ev)
    {
        if (ev->button() == Qt::LeftButton)
        {
            emit Clicked();
        }
        QWidget::mousePressEvent(ev);
    }

    void GradientWidget::paintEvent(QPaintEvent*)
    {
        QPixmap pxr(width(), height());
        pxr.fill(Qt::transparent);
        QPainter painter(&pxr);
        painter.setPen(Qt::NoPen);

        for (Gradient* gradient : m_gradients)
        {
            painter.setCompositionMode(gradient->m_mode);
            painter.setBrush(*gradient->m_gradient);
            painter.drawRect(rect());
        }
        QPainter m_painter(this);
        m_painter.fillRect(rect(), QBrush(m_background));
        m_painter.drawPixmap(0, 0, pxr);
    }

    void GradientWidget::AddKey(unsigned int id, QGradientStop stop)
    {
        if (!(id < static_cast<unsigned int>(m_gradients.count())))
        {
            return;
        }
        m_gradients[id]->m_gradient->setColorAt(stop.first, stop.second);
        update();
    }

    void GradientWidget::AddKey(unsigned int id, qreal stop, QColor color)
    {
        m_gradients[id]->m_gradient->setColorAt(stop, color);
        update();
    }

    void GradientWidget::SetKeys(unsigned int id, QGradientStops stops)
    {
        if (!(id < static_cast<unsigned int>(m_gradients.count())))
        {
            return;
        }
        m_gradients[id]->m_gradient->setStops(stops);
        update();
    }

    void GradientWidget::SetBackground(QString str)
    {
        m_background.load(str);
    }

    unsigned int GradientWidget::AddGradient(QGradient* gradient, QPainter::CompositionMode mode)
    {
        unsigned int index = m_gradients.count();
        m_gradients.push_back(new Gradient(gradient, mode));
        update();
        return index;
    }

    void GradientWidget::RemoveGradient(unsigned int id)
    {
        if (!(id < static_cast<unsigned int>(m_gradients.count())))
        {
            return;
        }
        m_gradients.remove(id);
        update();
    }

#include <moc_GradientWidget.cpp>
} // namespace OpenParticleSystemEditor
