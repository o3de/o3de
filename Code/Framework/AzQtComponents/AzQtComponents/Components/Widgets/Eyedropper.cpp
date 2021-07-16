/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QToolButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>

#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/MouseHider.h>
#include <AzQtComponents/Utilities/ScreenGrabber.h>

namespace AzQtComponents
{

    Eyedropper::Config Eyedropper::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("ContextSizeInPixels"), config.contextSizeInPixels);
        ConfigHelpers::read<int>(settings, QStringLiteral("ZoomFactor"), config.zoomFactor);

        return config;
    }

    Eyedropper::Config Eyedropper::defaultConfig()
    {
        return {
            15, // contextSizeInPixels
            8   // zoomFactor
        };
    }

    bool Eyedropper::polish(Style* style, QWidget* widget, const Eyedropper::Config& config)
    {
        bool polished = false;

        Eyedropper* instance = qobject_cast<Eyedropper*>(widget);
        if (instance != nullptr)
        {
            instance->setContextAndZoom(config.contextSizeInPixels, config.zoomFactor);

            style->repolishOnSettingsChange(instance);

            polished = true;
        }

        return polished;
    }

    Eyedropper::Eyedropper(QWidget* parent, QToolButton* button)
        : QWidget(parent, Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint | Qt::ToolTip)
        , m_button(button)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);
    }

    Eyedropper::~Eyedropper()
    {
    }

    bool Eyedropper::eventFilter(QObject*, QEvent* event)
    {
        switch (event->type()) {
            case QEvent::MouseMove:
                handleMouseMove();
                return true;
            case QEvent::MouseButtonRelease:
            {
                QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
                if (!mouse->buttons().testFlag(Qt::LeftButton))
                {
                    handleMouseMove();
                    release(true);
                    return true;
                }
            }
            case QEvent::KeyPress:
                handleKeyPress(static_cast<QKeyEvent*>(event));
                return true;
            default:
                break;
        }
        return false;
    }

    void Eyedropper::showEvent(QShowEvent *event)
    {
        m_mouseHider.reset(new MouseHider(this));
        m_grabber.reset(new ScreenGrabber(QSize{ m_sampleSize, m_sampleSize }, this));
        QCoreApplication::instance()->installEventFilter(this);
        QWidget::showEvent(event);
        handleMouseMove();
    }

    void Eyedropper::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // The midline of the outline/border
        // We clip to this and also base the rendering of the border off it
        int halfOutlineWidth = m_zoomFactor / 2;
        QMargins halfOutlineMargin = QMargins{ halfOutlineWidth, halfOutlineWidth, halfOutlineWidth, halfOutlineWidth };

        // Set up clipping
        QPainterPath path;
        path.addEllipse(rect() - halfOutlineMargin);
        painter.setClipPath(path);
        painter.setClipping(true);

        // Paint the zoomed image
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(rect(), m_sample);

        // Draw the grid
        QPen pen(QColor(0x80, 0x80, 0x80, 0x80));
        pen.setCosmetic(true);
        painter.setPen(pen);

        for (int x = m_zoomFactor; x < m_size; x += m_zoomFactor)
        {
            painter.drawLine(QPoint{x, 0}, QPoint{x, m_size});
        }

        for (int y = m_zoomFactor; y < m_size; y += m_zoomFactor)
        {
            painter.drawLine(QPoint{0, y}, QPoint{m_size, y});
        }

        pen.setCosmetic(false);
        
        // Draw the border of the hovered pixel
        QColor highlight = m_color.lightness() > 127 ? Qt::black : Qt::white;
        QColor lowlight = m_color.lightness() > 127 ? Qt::white : Qt::black;

        pen.setColor(highlight);
        pen.setWidth(2);
        painter.setPen(pen);
        int offset = m_contextSize * m_zoomFactor;
        painter.drawRoundedRect(offset, offset, m_zoomFactor, m_zoomFactor, 1.0, 1.0);

        // Draw color information
        QString text = QString("R:%1 G:%2 B:%3 A:%4 %5")
                .arg(m_color.red()).arg(m_color.green()).arg(m_color.blue()).arg(m_color.alpha()).arg(m_color.name(QColor::HexRgb).toUpper());

        const QFont font = painter.font();
        QFontMetrics metrics(font);
        QRect textBounds = metrics.boundingRect(text);
        textBounds += QMargins{3, 3, 3, 3};
        textBounds.moveCenter(QPoint{ m_centerOffset, m_centerOffset + textBounds.height()});

        QPainterPath box;
        box.addRoundedRect(textBounds, 1.0, 1.0);
        painter.fillPath(box, highlight);

        painter.setPen(lowlight);
        painter.drawText(textBounds, Qt::AlignHCenter | Qt::AlignVCenter, text);

        // Draw the outer border
        painter.setClipping(false);

        QRect bounds = rect() - halfOutlineMargin;
        pen.setColor(Qt::black);
        pen.setWidth(m_zoomFactor);
        painter.setPen(pen);
        painter.drawEllipse(bounds);

        pen.setColor(Qt::white);
        pen.setWidth(m_zoomFactor - 2);
        painter.setPen(pen);
        painter.drawEllipse(bounds);
    }

    void Eyedropper::setContextAndZoom(int contextSize, int zoomFactor)
    {
        m_contextSize = contextSize;
        m_sampleSize = 2 * contextSize + 1; // context in on each side + central pixel
        m_zoomFactor = zoomFactor;
        m_size = m_sampleSize * m_zoomFactor;
        m_centerOffset = m_sampleSize * m_zoomFactor / 2;

        setFixedSize(m_size, m_size);
    }

    void Eyedropper::handleMouseMove()
    {
        // if the grabber is already released then we should do nothing here
        if (!m_grabber)
        {
            return;
        }

        MappedPoint position = MappedCursorPosition();

        m_sample = m_grabber->grab(position.native);
        m_color = m_sample.pixelColor(m_sample.rect().center());
        emit colorHovered(m_color);

        QSize half = size() / 2;
        move(position.qt - QPoint{ half.width(), half.height() });
        update();
    }

    void Eyedropper::handleKeyPress(QKeyEvent* event)
    {
        if (event->matches(QKeySequence::Cancel))
        {
            release(false);
        }
        else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            release(true);
        }
        event->accept();
    }

    void Eyedropper::release(bool selected)
    {
        if (selected)
        {
            emit colorSelected(m_color);
        }

        hide();
        QCoreApplication::instance()->removeEventFilter(this);
        m_grabber.reset(nullptr);
        m_mouseHider.reset(nullptr);
    }

} // namespace AzQtComponents

#include "Components/Widgets/moc_Eyedropper.cpp"
