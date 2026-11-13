/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Window/ParticleLineWidget.h>
#include <Window/ParticleEditorFlat.h>
#include <Window/ParticleItemWidget.h>

namespace OpenParticleSystemEditor
{
    ParticleLineWidget::ParticleLineWidget(QWidget* parent)
        : QWidget(parent)
        , m_index(WIDGET_LINE_EMITTER)
    {
        // The row paints its own fill. Qt's style sheet style gave a widget of QWidget's own meta object a styled background when it
        // polished it (the legacy GUI), which a class with Q_OBJECT does not get: the attribute says it, in both GUIs
        setAttribute(Qt::WA_StyledBackground, true);

        // The idle row: the sheet of the legacy GUI, and in the flattened GUI the row's element (ParticleEditorFlat)
        ParticleEditorFlat::SetLineRowPressed(this, false);
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
            ParticleEditorFlat::SetLineRowPressed(this, true);
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
            ParticleEditorFlat::SetLineRowPressed(this, false);
        }
    };
} // namespace OpenParticleSystemEditor


