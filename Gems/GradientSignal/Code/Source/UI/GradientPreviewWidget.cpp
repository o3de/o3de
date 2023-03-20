/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UI/GradientPreviewWidget.h>

#include <QIcon>
#include <QPainter>
#include <QToolButton>
#include <QVBoxLayout>

namespace GradientSignal
{
    GradientPreviewWidget::GradientPreviewWidget(QWidget* parent, bool enablePopout)
        : QWidget(parent)
    {
        setMinimumSize(256, 256);
        setAttribute(Qt::WA_OpaquePaintEvent); // We're responsible for painting everything, don't bother erasing before paint

        // For the preview with popout icon, configure an icon in the top-right
        // corner of our preview that only appears when the user hovers over
        // the preview
        if (enablePopout)
        {
            const int IconMargin = 2;
            const int IconSize = 24;

            QVBoxLayout* layout = new QVBoxLayout(this);
            layout->setContentsMargins(QMargins(IconMargin, IconMargin, IconMargin, IconMargin));
            layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

            QIcon icon;
            icon.addPixmap(QPixmap(":/Application/popout-overlay.svg"), QIcon::Normal);
            icon.addPixmap(QPixmap(":/Application/popout-overlay-hover.svg"), QIcon::Active);

            m_popoutButton = new QToolButton(this);
            m_popoutButton->setIcon(icon);
            m_popoutButton->setAutoRaise(true);
            m_popoutButton->setIconSize(QSize(IconSize, IconSize));
            m_popoutButton->setCursor(Qt::PointingHandCursor);
            m_popoutButton->hide();
            layout->addWidget(m_popoutButton);
            QObject::connect(m_popoutButton, &QToolButton::clicked, this, &GradientPreviewWidget::popoutClicked);
        }
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

    void GradientPreviewWidget::enterEvent(QEvent* event)
    {
        QWidget::enterEvent(event);

        if (m_popoutButton)
        {
            m_popoutButton->show();
        }
    }

    void GradientPreviewWidget::leaveEvent(QEvent* event)
    {
        QWidget::leaveEvent(event);

        if (m_popoutButton)
        {
            m_popoutButton->hide();
        }
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

#include "UI/moc_GradientPreviewWidget.cpp"
