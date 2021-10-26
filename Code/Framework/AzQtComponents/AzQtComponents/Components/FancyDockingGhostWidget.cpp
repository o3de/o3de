/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/FancyDockingGhostWidget.h>

#include <QApplication>
#include <QDebug>
#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QMainWindow>
#include <QRect>
#include <QPainter>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

namespace AzQtComponents
{
    FancyDockingGhostWidget::FancyDockingGhostWidget(QMainWindow* mainWindow, QWidget* parent) :
        QWidget(parent, Qt::WindowFlags(Qt::ToolTip | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint)),
        m_mainWindow(mainWindow)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
    }

    FancyDockingGhostWidget::~FancyDockingGhostWidget()
    {
    }

    void FancyDockingGhostWidget::setPixmapVisible(bool visible)
    {
        if (m_visible != visible)
        {
            m_visible = visible;
            if (visible)
            {
                update();
            }
        }
    }

    void FancyDockingGhostWidget::setPixmap(const QPixmap& pixmap, const QRect& targetRect, QScreen* screen)
    {
        bool needsRepaint = m_pixmap.cacheKey() != pixmap.cacheKey() || m_clipToWidgets;
        m_pixmap = pixmap;

        if (pixmap.isNull() || targetRect.isNull() || !screen)
        {
            setPixmapVisible(false);
            return;
        }

        QWindow *window = windowHandle();
        if (!window)
        {
            // So we don't crash when setting a pixmap before having a window
            create();
            window = windowHandle();
        }

        if (window->screen() != screen)
        {
            // Qt usually handles screens with different scale factors correctly, however
            // when the geometry's origin is in a gap between monitors it won't map to native
            // coordinates correctly, we have to set the screen before setting geometry, so that
            // Qt uses the correct scale factor when mapping to native coordinates.
            // (By gap we mean that monitors are not consecutive, they might be consecutive
            // in native coordinates, but due to using scale factors there will be gaps that don't
            // belong to any screen.
            window->setScreen(screen);
        }

        QPoint midPoint = targetRect.topLeft() + QPoint(targetRect.width() / 2, targetRect.height() / 2);
        QScreen* pointScreen = QApplication::screenAt(midPoint);
        QRect rect(targetRect);

        // In environments with multiple screens the screen coordinate system may have gaps, especially when different scaling settings
        // are involved. When that happens, if a widget is moved into the gap it will resize and translate with undefined behavior.
        // To prevent this, whenever the widget would end up outside screen boundaries, we resize the widget to be twice its
        // original size so that the center of the widget is back inside the screen boundaries, and set the ghost widget
        // to paint the widget pixmap at half the previous size to make the process seamless.
        // This makes the dragging a lot smoother in most situations.
        PaintMode paintMode = PaintMode::FULL;

        if (!pointScreen || pointScreen != screen)
        {
            if (midPoint.x() >= QCursor::pos().x())
            {
                rect.setLeft(rect.left() - rect.width());
                rect.setTop(rect.top() - rect.height());
                paintMode = PaintMode::BOTTOMRIGHT;
            }
            else
            {
                rect.setRight(rect.right() + rect.width());
                rect.setTop(rect.top() - rect.height());
                paintMode = PaintMode::BOTTOMLEFT;
            }
        }

        if (m_paintMode != paintMode)
        {
            needsRepaint = true;
        }

        setGeometry(rect);
        m_paintMode = paintMode;

        setPixmapVisible(true);
        if (needsRepaint)
        {
            // We use repaint instead of update since the latter has a delay of 1 frame,
            // which would cause the ghost widget to flicker when changing paint mode.
            repaint();
        }
    }

    // The equivalent of lowering the pixmap under the parent's dock widgets
    void FancyDockingGhostWidget::EnableClippingToDockWidgets()
    {
        if (!m_clipToWidgets)
        {
            m_clipToWidgets = true;
            update();
        }
    }

    // The equivalent of raising above all of the parent's dock widgets
    void FancyDockingGhostWidget::DisableClippingToDockWidgets()
    {
        if (m_clipToWidgets)
        {
            m_clipToWidgets = false;
            update();
        }
    }

    void FancyDockingGhostWidget::closeEvent(QCloseEvent* ev)
    {
        ev->ignore(); // Don't close the window.
    }

    void FancyDockingGhostWidget::paintEvent(QPaintEvent* /* ev */)
    {
        QPainter painter(this);

        if (m_visible && !m_pixmap.isNull())
        {
            if (m_clipToWidgets)
            {
                painter.save();
                AzQtComponents::SetClipRegionForDockingWidgets(this, painter, m_mainWindow);
            }

            // Our rendered pixmaps might be smaller than the widget size
            // We put them at the bottom of the widget.
            // It's specifically because we use this widget to render
            // floating dock widgets, and they often render without the titlebar
            // but the geometry of the widget will take the title bar into account.
            QSize widgetSize = rect().size();
            int aspectRatioHeight = (m_pixmap.height() * widgetSize.width()) / m_pixmap.width();
            int yOffset = 0;

            if (aspectRatioHeight < widgetSize.height())
            {
                yOffset = widgetSize.height() - aspectRatioHeight;
            }

            switch (m_paintMode)
            {
                case PaintMode::FULL:
                    {
                        painter.drawPixmap(QRect(0, yOffset, widgetSize.width(), aspectRatioHeight), m_pixmap);
                    }
                break;

                case PaintMode::BOTTOMLEFT:
                    {
                        painter.drawPixmap(QRect(0, (widgetSize.height() + yOffset) / 2, widgetSize.width() / 2, aspectRatioHeight / 2), m_pixmap);
                    }
                break;

                case PaintMode::BOTTOMRIGHT:
                    {
                        painter.drawPixmap(QRect(widgetSize.width() / 2, (widgetSize.height() + yOffset) / 2, widgetSize.width() / 2, aspectRatioHeight / 2), m_pixmap);
                    }
                break;
            }

            if (m_clipToWidgets)
            {
                painter.restore();
            }
        }
        else
        {
            // fill to blank, in case anything is cached
            painter.fillRect(rect(), QColor(0, 0, 0, 0));
        }
    }
}

