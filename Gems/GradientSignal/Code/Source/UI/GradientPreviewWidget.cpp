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
