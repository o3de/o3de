/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include <UI/GradientPreviewWidget.h>

#include <QPainter>

namespace GradientSignal
{
    GradientPreviewWidget::GradientPreviewWidget(QWidget* parent)
        : QWidget(parent)
    {
        setMinimumSize(256, 256);
        setAttribute(Qt::WA_OpaquePaintEvent); // We're responsible for painting everything, don't bother erasing before paint
    }

    GradientPreviewWidget::~GradientPreviewWidget()
    {
    }

    void GradientPreviewWidget::OnUpdate()
    {
        update();
    }

    QSize GradientPreviewWidget::GetPreviewSize() const
    {
        return size();
    }

    void GradientPreviewWidget::paintEvent([[maybe_unused]] QPaintEvent* paintEvent)
    {
        QPainter painter(this);
        if (!m_previewImage.isNull())
        {
            painter.drawImage(QPoint(0, 0), m_previewImage);
        }
    }

    void GradientPreviewWidget::resizeEvent(QResizeEvent* resizeEvent)
    {
        QWidget::resizeEvent(resizeEvent);

        QueueUpdate();
    }
} //namespace GradientSignal
