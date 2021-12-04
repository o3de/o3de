/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>

#include <QMainWindow>
#include <QCloseEvent>
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

namespace AzQtComponents
{

    FancyDockingDropZoneWidget::FancyDockingDropZoneWidget(QMainWindow* mainWindow, QWidget* coordinatesRelativeTo, QScreen* screen, FancyDockingDropZoneState* dropZoneState)
        // NOTE: this will not work with multiple monitors if this widget has a parent. The floating drop zone
        // won't render on anything other than the parent's screen for some reason, if the parent is set.
        : QWidget(nullptr, Qt::WindowFlags(Qt::ToolTip | Qt::BypassWindowManagerHint | Qt::FramelessWindowHint))
        , m_mainWindow(mainWindow)
        , m_relativeTo(coordinatesRelativeTo)
        , m_screen(screen)
        , m_dropZoneState(dropZoneState)
    {
        m_dropZoneState->registerListener(this);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        setGeometry(screen->availableGeometry());

        Stop();
    }

    FancyDockingDropZoneWidget::~FancyDockingDropZoneWidget()
    {
        m_dropZoneState->unregisterListener(this);
    }

    QScreen* FancyDockingDropZoneWidget::GetScreen()
    {
        return m_screen;
    }

    void FancyDockingDropZoneWidget::Start()
    {
        QWindow *window = windowHandle();
        if (!window)
        {
            // So we don't crash when setting a pixmap before having a window
            create();
            window = windowHandle();
        }

        if (window->screen() != m_screen)
        {
            // Qt usually handles screens with different scale factors correctly, however
            // when the geometry's origin is in a gap between monitors it won't map to native
            // coordinates correctly, we have to set the screen before setting geometry, so that
            // Qt uses the correct scale factor when mapping to native coordinates.

            // (By gap we mean that monitors are not consecutive, they might be consecutive
            // in native coordinates, but due to using scale factors there will be gaps that don't
            // belong to any screen.
            window->setScreen(m_screen);
        }

        QRect screenGeometry = window->screen()->geometry();
        if (screenGeometry != geometry())
        {
            setGeometry(screenGeometry);
        }

        show();
    }

    void FancyDockingDropZoneWidget::Stop()
    {
#ifdef AZ_PLATFORM_MAC
        // macOS needs a bit help, WA_TransparentForMouseEvents doesn't always work
        // there's a rare edge case when dragging while a popup is open which leads to events being sent to this window
        // even if it's not visible. As far as I can tell it's not a Qt bug, the OS sends the events to this window.
        // So here's this workaround
        setGeometry(0, 0, 1, 1);
#endif
        hide();
    }

    void FancyDockingDropZoneWidget::paintEvent(QPaintEvent* ev)
    {
        (void)ev;

        if (!isVisible())
        {
            return;
        }

        if (!m_dropZoneState->dragging())
        {
            return;
        }

        bool modifiedKeyPressed = CheckModifierKey();

        if (!modifiedKeyPressed)
        {
            QPainter painter(this);

            QWidget* dropOnto = m_dropZoneState->dropOnto();
            if (dropOnto)
            {
                QMainWindow* mainWindow = qobject_cast<QMainWindow*>(dropOnto);
                if (!mainWindow)
                {
                    mainWindow = qobject_cast<QMainWindow*>(dropOnto->parentWidget());
                }

                // If our drop target isn't a QMainWindow, then retrieve the base Editor QMainWindow.
                // Since *this* widget is not properly parented against the FancyDocking widget,
                // (which is because Qt doesn't support rendering across multiple screens properly)
                // we need to manually clip against any floating windows.
                if (!mainWindow || mainWindow == m_mainWindow)
                {
                    AzQtComponents::SetClipRegionForDockingWidgets(this, painter, m_mainWindow);
                }
            }

            // Draw all of the normal drop zones if they exist (if a dock widget is hovered over)
            painter.setPen(Qt::NoPen);
            painter.setOpacity(FancyDockingDropZoneConstants::dropZoneOpacity);
            auto dropZones = m_dropZoneState->dropZones();
            for (auto it = dropZones.cbegin(); it != dropZones.cend(); ++it)
            {
                const Qt::DockWidgetArea area = it.key();
                const QPolygon& dropZoneShape = it.value();

                paintDropZone(area, dropZoneShape, painter);
            }

            // Draw the absolute drop zone and drop borders if they exist
            fillAbsoluteDropZone(painter);
            paintDropBorderLines(painter);
        }
    }

    void FancyDockingDropZoneWidget::closeEvent(QCloseEvent* ev)
    {
        ev->ignore(); // Don't close the window.
    }

    void FancyDockingDropZoneWidget::paintDropZone(const Qt::DockWidgetArea area, QPolygon dropZoneShape, QPainter& painter)
    {
        painter.save();

        // If this drop zone is currently hovered over, then set the on hover color
        // and the hover opacity as it fades in
        if (area == m_dropZoneState->dropArea() && !m_dropZoneState->onAbsoluteDropZone())
        {
            painter.setOpacity(m_dropZoneState->dropZoneHoverOpacity());
            painter.setBrush(m_dropZoneState->dropZoneColorOnHover());
        }
        // Otherwise, set the normal color
        else
        {
            painter.setBrush(FancyDockingDropZoneConstants::dropZoneColor);
        }

        // negate the window position to offset everything by that much
        QPoint offset = mapFromGlobal(m_relativeTo->mapToGlobal(QPoint(0, 0)));

        dropZoneShape = dropZoneShape.translated(offset);

        // If this is the center tab drop zone, then we need to draw a circle and the tabs icon
        if (area == Qt::AllDockWidgetAreas)
        {
            // Use antialiasing to make sure that ellipses aren't jagged
            painter.setRenderHint(QPainter::Antialiasing);

            // If the center drop zone isn't currently hovered over, then draw the
            // circle first so that the tab icon is drawn on top
            const QRect& dropZoneRect = dropZoneShape.boundingRect();
            if (area != m_dropZoneState->dropArea())
            {
                painter.drawEllipse(dropZoneRect);
            }

            // Scale the tabs icon based on the drop zone size and our specified offset
            // Doing this through QIcon to make sure that SVG is rendered already in desired resolution
            const QSize& dropZoneSize = dropZoneRect.size();
            const QSize requestedIconSize = dropZoneSize * FancyDockingDropZoneConstants::centerTabIconScale;
            const QIcon dropZoneIcon = QIcon(FancyDockingDropZoneConstants::centerDropZoneIconPath);
            const QPixmap dropZonePixmap = dropZoneIcon.pixmap(requestedIconSize);
            const QSize receivedIconSize = dropZoneIcon.actualSize(requestedIconSize);

            // Draw the icon in the center of the drop zone with full opacity
            const QPoint& dropZoneCenter = dropZoneRect.center();
            int tabsIconX = dropZoneCenter.x() - (receivedIconSize.width() / 2);
            int tabsIconY = dropZoneCenter.y() - (receivedIconSize.height() / 2);
            qreal opacity = painter.opacity();
            painter.setOpacity(1);
            painter.drawPixmap(tabsIconX, tabsIconY, dropZonePixmap);

            // If the center drop zone is currently hovered over, then draw the
            // circle for the drop zone after the tab icon so it gets drawn on
            // top so they both get the hover color
            if (area == m_dropZoneState->dropArea())
            {
                painter.setOpacity(opacity);
                painter.drawEllipse(dropZoneRect);
            }
        }
        // Otherwise just draw the trapezoid for the drop zone
        else
        {
            painter.drawPolygon(dropZoneShape);
        }

        painter.restore();
    }

    void FancyDockingDropZoneWidget::fillAbsoluteDropZone(QPainter& painter)
    {
        // Draw the absolute drop zone if it is valid with the proper color (on hover vs normal)
        if (shouldFillAbsoluteDropZone())
        {
            // negate the window position to offset everything by that much
            QPoint offset = mapFromGlobal(m_relativeTo->mapToGlobal(QPoint(0, 0)));

            QRect absoluteDropZoneRect = m_dropZoneState->absoluteDropZoneRect().translated(offset);

            painter.save();

            if (m_dropZoneState->onAbsoluteDropZone())
            {
                painter.setOpacity(m_dropZoneState->dropZoneHoverOpacity());
                painter.setBrush(m_dropZoneState->dropZoneColorOnHover());
            }
            else
            {
                painter.setBrush(FancyDockingDropZoneConstants::dropZoneColor);
            }
            painter.drawRect(absoluteDropZoneRect);

            painter.restore();
        }
    }

    bool FancyDockingDropZoneWidget::shouldFillAbsoluteDropZone() const
    {
        // Draw the absolute drop zone if it is valid with the proper color (on hover vs normal)
        return m_dropZoneState->absoluteDropZoneRect().isValid();
    }

    bool FancyDockingDropZoneWidget::shouldPaintDropBorderLines() const
    {
        // Don't draw the border lines if we don't have a valid drop target, or if
        // we don't have multiple drop zones to draw (if there's only one drop zone, it means
        // we only have the center tab drop zone)
        return m_dropZoneState->dropOnto() && m_dropZoneState->dropZones().size() > 1;
    }

    void FancyDockingDropZoneWidget::paintDropBorderLines(QPainter& painter)
    {
        // Don't draw the border lines if we don't have a valid drop target, or if
        // there are no normal drop zones
        if (!shouldPaintDropBorderLines())
        {
            return;
        }

        // Retrieve the outer (dock widget) and inner corner points so that we can draw the lines
        // separating their borders.
        QWidget* dropOnto = m_dropZoneState->dropOnto();
        QRect dockDropZoneRect = m_dropZoneState->dockDropZoneRect();
        const QPoint topLeft = mapFromGlobal(dropOnto->mapToGlobal(dockDropZoneRect.topLeft()));
        const QPoint topRight = mapFromGlobal(dropOnto->mapToGlobal(dockDropZoneRect.topRight()));
        const QPoint bottomLeft = mapFromGlobal(dropOnto->mapToGlobal(dockDropZoneRect.bottomLeft()));
        const QPoint bottomRight = mapFromGlobal(dropOnto->mapToGlobal(dockDropZoneRect.bottomRight()));

        // negate the window position to offset everything by that much
        QPoint offset = mapFromGlobal(m_relativeTo->mapToGlobal(QPoint(0, 0)));
        QRect innerDropZoneRect = m_dropZoneState->innerDropZoneRect().translated(offset);
        const QPoint innerTopLeft = innerDropZoneRect.topLeft();
        const QPoint innerTopRight = innerDropZoneRect.topRight();
        const QPoint innerBottomLeft = innerDropZoneRect.bottomLeft();
        const QPoint innerBottomRight = innerDropZoneRect.bottomRight();

        // Draw the lines using the appropriate pen
        QPen dropZoneBorderPen(FancyDockingDropZoneConstants::dropZoneBorderColor);
        dropZoneBorderPen.setWidth(FancyDockingDropZoneConstants::dropZoneBorderInPixels);
        painter.setPen(dropZoneBorderPen);
        painter.setOpacity(1);
        painter.drawLine(topLeft, innerTopLeft);
        painter.drawLine(topRight, innerTopRight);
        painter.drawLine(bottomLeft, innerBottomLeft);
        painter.drawLine(bottomRight, innerBottomRight);

        // If we have a valid absolute drop zone, then draw a border line between it and the drop zone it shares a side with
        if (m_dropZoneState->absoluteDropZoneRect().isValid())
        {
            switch (m_dropZoneState->absoluteDropZoneArea())
            {
            case Qt::LeftDockWidgetArea:
                painter.drawLine(topLeft, bottomLeft);
                break;
            case Qt::RightDockWidgetArea:
                painter.drawLine(topRight, bottomRight);
                break;
            case Qt::TopDockWidgetArea:
                painter.drawLine(topLeft, topRight);
                break;
            case Qt::BottomDockWidgetArea:
                painter.drawLine(bottomLeft, bottomRight);
                break;
            }
        }
    }

    bool FancyDockingDropZoneWidget::CheckModifierKey()
    {
        // use query instead of keyboardModifiers() so that it queries the actual state,
        // instead of the state as recorded by events processed so far.
        // Slower, but more accurate when we're dragging the window
        return (QGuiApplication::queryKeyboardModifiers() & Qt::ControlModifier);
    }

} // namespace AzQtComponents

#include "Components/moc_FancyDockingDropZoneWidget.cpp"
