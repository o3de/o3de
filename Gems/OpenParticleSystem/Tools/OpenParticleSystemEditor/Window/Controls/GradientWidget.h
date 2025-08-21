/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidget>
#include <QGradient>
#include <QPaintEvent>
#include <QPainter>

namespace OpenParticleSystemEditor
{
    class GradientWidget : public QWidget
    {
        Q_OBJECT
    public:
        GradientWidget(QWidget* parent = 0);
        virtual ~GradientWidget();
        virtual void paintEvent(QPaintEvent*) override;

        unsigned int AddGradient(QGradient* gradient, QPainter::CompositionMode mode);
        void RemoveGradient(unsigned int id);
        void SetKeys(unsigned int id, QGradientStops stops);
        void AddKey(unsigned int id, qreal stop, QColor color);
        void AddKey(unsigned int id, QGradientStop stop);
        void SetBackground(QString str);

    signals:
        void Clicked();

    protected:
        void mousePressEvent(QMouseEvent* ev) override;
        struct Gradient
        {
            QGradient* m_gradient;
            QPainter::CompositionMode m_mode;

            explicit Gradient(QGradient* gradient, QPainter::CompositionMode mode = QPainter::CompositionMode::CompositionMode_Plus)
                : m_gradient(gradient)
                , m_mode(mode)
            {
                gradient->setCoordinateMode(QGradient::ObjectBoundingMode);
            }

            explicit Gradient(const Gradient& other)
            {
                m_gradient = other.m_gradient;
                m_mode = other.m_mode;
            }

            ~Gradient()
            {
                if (m_gradient != nullptr)
                {
                    delete m_gradient;
                    m_gradient = nullptr;
                }
            }

            Gradient& operator=(const Gradient& gradient)
            {
                if (&gradient == this)
                {
                    return *this;
                }
                m_gradient = gradient.m_gradient;
                m_mode = gradient.m_mode;
                return *this;
            }
        };
        QPixmap m_background;
        QVector<Gradient*> m_gradients;
    };
} // namespace OpenParticleSystemEditor
