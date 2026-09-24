/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include "Tools/WhiteBoxToolStatus.h"

#include <QApplication>
#include <QFontMetrics>
#include <QLabel>
#include <QVBoxLayout>

namespace WhiteBox
{
    WhiteBoxToolStatus::WhiteBoxToolStatus(QWidget* viewport)
        : QFrame(viewport, Qt::Tool | Qt::FramelessWindowHint |
              Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput)
        , m_viewport(viewport)
        , m_placementTimer(this)
    {
        setObjectName(QStringLiteral("WhiteBoxToolStatus"));
        setAttribute(Qt::WA_ShowWithoutActivating);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_WindowPropagation);
        setFocusPolicy(Qt::NoFocus);
        setFrameShape(QFrame::NoFrame); // the fill already separates it from the scene
        setAutoFillBackground(true);

        auto* rows = new QVBoxLayout(this);
        rows->setContentsMargins(6, 2, 6, 2);
        rows->setSpacing(1);
        const auto addLabel = [this, rows]()
        {
            auto* label = new QLabel(this);
            label->setTextFormat(Qt::PlainText);
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::NoTextInteraction);
            label->setFocusPolicy(Qt::NoFocus);
            rows->addWidget(label);
            return label;
        };
        m_instructions = addLabel();
        m_error = addLabel();
        m_error->hide();

        // Track docking, resizing, moving the editor and application activation, even when the
        // viewport does not repaint. Only active tools run this timer; colours/fonts are inherited.
        m_placementTimer.setInterval(80);
        connect(&m_placementTimer, &QTimer::timeout, this, [this]() { UpdatePlacement(); });
    }

    void WhiteBoxToolStatus::SetStatus(const QString& instructions, const QString& error)
    {
        const QString errorText = error.isEmpty() ? QString{} : tr("Notice: %1").arg(error);
        const bool changed = m_instructions->text() != instructions || m_error->text() != errorText;
        if (changed)
        {
            m_instructions->setText(instructions);
            m_error->setText(errorText);
            m_error->setVisible(!error.isEmpty());
        }
        const bool starting = !m_requestedVisible;
        m_requestedVisible = true;
        if (!m_placementTimer.isActive()) { m_placementTimer.start(); }
        if (starting || changed) { UpdatePlacement(); }
    }

    void WhiteBoxToolStatus::HideStatus()
    {
        m_requestedVisible = false;
        m_placementTimer.stop();
        hide();
    }

    void WhiteBoxToolStatus::UpdatePlacement()
    {
        if (!m_requestedVisible || !m_viewport || !m_viewport->isVisible() ||
            m_viewport->window()->isMinimized() || QApplication::applicationState() != Qt::ApplicationActive)
        {
            hide();
            return;
        }

        // A step smaller than the viewport's own text, because this is a hint strip rather than a
        // heading. Taken from the viewport font every pass so it keeps following the UI scale.
        QFont compact = m_viewport->font();
        if (compact.pointSizeF() > 0.0)
        {
            compact.setPointSizeF(qMax(compact.pointSizeF() * 0.85, 7.0));
        }
        else if (compact.pixelSize() > 0)
        {
            compact.setPixelSize(qMax(qRound(compact.pixelSize() * 0.85), 9));
        }
        if (font() != compact)
        {
            setFont(compact);
        }

        // Qt widget coordinates are device independent, and font-relative spacing follows the UI
        // scale. The strip sits just above the bottom edge, clear of whatever the viewport draws there.
        const QFontMetrics metrics(font());
        const int margin = qMax(8, metrics.height() / 2);
        const int bottomClearance = margin; // sits down on the viewport's bottom edge, not above it
        const int availableWidth = m_viewport->width() - margin * 2;
        const int availableHeight = m_viewport->height() - bottomClearance - margin;
        if (availableWidth < 120 || availableHeight < metrics.height() + 8)
        {
            hide();
            return;
        }
        // Fit the actual text instead of reserving a fixed-width panel. Long instructions and
        // notices still wrap when the viewport is narrow.
        const int textWidth = qMax(
            m_instructions->fontMetrics().horizontalAdvance(m_instructions->text()),
            m_error->fontMetrics().horizontalAdvance(m_error->text()));
        const auto margins = layout()->contentsMargins();
        const int naturalWidth = textWidth + margins.left() + margins.right() + frameWidth() * 2;
        const int statusWidth = qMin(availableWidth, naturalWidth);
        if (width() != statusWidth) { setFixedWidth(statusWidth); }
        layout()->activate();
        const int wrappedHeight = layout()->totalHeightForWidth(statusWidth);
        const int statusHeight = qMin(availableHeight,
            wrappedHeight >= 0 ? wrappedHeight : layout()->totalSizeHint().height());
        const QPoint position = m_viewport->mapToGlobal(
            QPoint(margin, m_viewport->height() - bottomClearance - statusHeight));
        const QRect placement(position, QSize(statusWidth, statusHeight));
        if (geometry() != placement) { setGeometry(placement); }
        if (!isVisible()) { show(); } // Never activate or focus: Enter/Esc still reach the viewport.
    }
} // namespace WhiteBox
