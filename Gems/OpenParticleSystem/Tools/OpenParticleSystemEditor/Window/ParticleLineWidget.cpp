/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/ParticleLineWidget.h>
#include <Window/ParticleItemWidget.h>

namespace OpenParticleSystemEditor
{
    ParticleLineWidget::ParticleLineWidget(QWidget* parent)
        : QWidget(parent)
        , m_index(WIDGET_LINE_EMITTER)
    {
        this->setStyleSheet("background-color: #474747; border-radius: 1px;");
    }

    ParticleLineWidget::~ParticleLineWidget()
    {
    }

    void ParticleLineWidget::SetLineWidgetParent(QWidget* widget)
    {
        m_parent = widget;
    }

    void ParticleLineWidget::SetLineIndex(LineWidgetIndex Index)
    {
        m_index = Index;
    }

    void ParticleLineWidget::mousePressEvent(QMouseEvent* event)
    {
        Q_UNUSED(event);
        if (m_index != WIDGET_LINE_TITLE)
        {
            this->setStyleSheet("background-color: #666666; border-radius: 1px;");
        }

        ParticleItemWidget* itemWidget = dynamic_cast<ParticleItemWidget*>(m_parent);
        if (itemWidget != nullptr)
        {
            itemWidget->ClickLineWidget(m_index);
        }
    }

    void ParticleLineWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        Q_UNUSED(event);
        if (m_index != WIDGET_LINE_TITLE)
        {
            this->setStyleSheet("background-color: #474747; border-radius: 1px;");
        }
    };
} // namespace OpenParticleSystemEditor


