/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <qglobal.h> // For qreal
#include <qnamespace.h> // For Qt::DockArea
#include <QColor>
#include <QString>
#include <QMap>
#include <QPointer>
#include <QRect>
#include <QWidget>
#include <QPolygon>
#endif

class QScreen;
class QMainWindow;
class QPainter;

namespace AzQtComponents
{
    namespace FancyDockingDropZoneConstants
    {
        // Constant for the opacity of the screen grab for the dock widget being dragged
        static constexpr qreal draggingDockWidgetOpacity = 0.6;

        // Constant for the opacity of the normal drop zones
        static constexpr qreal dropZoneOpacity  = 0.4;

        // Constant for the default drop zone size (in pixels)
        static constexpr int dropZoneSizeInPixels  = 40;

        // Constant for the dock width/height size (in pixels) before we need to start
        // scaling down the drop zone sizes, or else they will overlap with the center
        // tab icon or each other
        static constexpr int minDockSizeBeforeDropZoneScalingInPixels = dropZoneSizeInPixels * 3;

        // Constant for the factor by which we must scale down the drop zone sizes once
        // the dock width/height size is too small
        static constexpr qreal dropZoneScaleFactor  = 0.25;

        // Constant for the percentage to scale down the inner drop zone rectangle for the center tab drop zone
        static constexpr qreal centerTabDropZoneScale  = 0.5;

        // Constant for the percentage to scale down the center tab drop zone for the center tab icon
        static constexpr qreal centerTabIconScale  = 0.5;

        // Constant for the drop zone hotspot default color
        static const QColor dropZoneColor = QColor(155, 155, 155);

        // Constant for the drop zone border color
        static const QColor dropZoneBorderColor = Qt::black;

        // Constant for the border width in pixels separating the drop zones
        static constexpr int dropZoneBorderInPixels  = 1;

        // Constant for the border width in pixels separating the drop zones
        static constexpr int absoluteDropZoneSizeInPixels  = 25;

        // Constant for the delay (in milliseconds) before a drop zone becomes active
        // once it is hovered over
        static constexpr int dockingTargetDelayMS = 110;

        // Constant for the rate at which we will update (fade in) the drop zone opacity
        // when hovered over (in milliseconds)
        static constexpr int dropZoneHoverFadeUpdateIntervalMS  = 20;

        // Constant for the incremental opacity increase for the hovered drop zone
        // that will fade in to the full drop zone opacity in the desired time
        static constexpr qreal dropZoneHoverFadeIncrement = dropZoneOpacity / (dockingTargetDelayMS / dropZoneHoverFadeUpdateIntervalMS);

        // Constant for the path to the center drop zone tabs icon
        static const QString centerDropZoneIconPath = QStringLiteral(":/stylesheet/img/UI20/docking/tabs_icon.svg");
    };

    class FancyDockingDropZoneState
    {
    public:

        const QMap<Qt::DockWidgetArea, QPolygon> dropZones() const
        {
            return m_dropZones;
        }

        void setDropZones(const QMap<Qt::DockWidgetArea, QPolygon>& zones)
        {
            if (zones != m_dropZones)
            {
                m_dropZones = zones;
                repaintDropZoneWidgets();
            }
        }

        bool hasDropZones() const
        {
            return !m_dropZones.isEmpty();
        }

        QPointer<QWidget> dropOnto() const
        {
            return m_dropOnto;
        }

        void setDropOnto(QWidget *widget)
        {
            if (m_dropOnto != widget)
            {
                m_dropOnto = widget;
                repaintDropZoneWidgets();
            }
        }

        Qt::DockWidgetArea dropArea() const
        {
            return m_dropArea;
        }

        void setDropArea(Qt::DockWidgetArea area)
        {
            if (m_dropArea != area)
            {
                m_dropArea = area;
                repaintDropZoneWidgets();
            }
        }

        qreal dropZoneHoverOpacity()
        {
            return m_dropZoneHoverOpacity;
        }

        void setDropZoneHoverOpacity(qreal opacity)
        {
            if (m_dropZoneHoverOpacity != opacity)
            {
                m_dropZoneHoverOpacity = opacity;
                repaintDropZoneWidgets();
            }
        }

        QColor dropZoneColorOnHover()
        {
            return m_dropZoneColorOnHover;
        }

        void setDropZoneColorOnHover(const QColor &color)
        {
            if (m_dropZoneColorOnHover != color)
            {
                m_dropZoneColorOnHover = color;
                repaintDropZoneWidgets();
            }
        }

        bool onAbsoluteDropZone() const
        {
            return m_onAbsoluteDropZone;
        }

        void setOnAbsoluteDropZone(bool absolute)
        {
            // Don't issue a repaint. This doesn't affect our painting and only used by FancyDocking.
            // FancyDocking toggles this per each mouse move event, would be a big CPU waste
            m_onAbsoluteDropZone = absolute;
        }

        QRect absoluteDropZoneRect() const
        {
            return m_absoluteDropZoneRect;
        }

        void setAbsoluteDropZoneRect(QRect rect)
        {
            if (m_absoluteDropZoneRect != rect)
            {
                m_absoluteDropZoneRect = rect;
                repaintDropZoneWidgets();
            }
        }

        Qt::DockWidgetArea absoluteDropZoneArea() const
        {
            return m_absoluteDropZoneArea;
        }

        void setAbsoluteDropZoneArea(Qt::DockWidgetArea area)
        {
            if (m_absoluteDropZoneArea != area)
            {
                m_absoluteDropZoneArea = area;
                repaintDropZoneWidgets();
            }
        }

        QRect dockDropZoneRect() const
        {
            return m_dockDropZoneRect;
        }

        void setDockDropZoneRect(QRect rect)
        {
            if (m_dockDropZoneRect != rect)
            {
                m_dockDropZoneRect = rect;
                repaintDropZoneWidgets();
            }
        }

        QRect innerDropZoneRect() const
        {
            return m_innerDropZoneRect;
        }

        void setInnerDropZoneRect(QRect rect)
        {
            if (m_innerDropZoneRect != rect)
            {
                m_innerDropZoneRect = rect;
                repaintDropZoneWidgets();
            }
        }

        bool dragging() const
        {
            return m_dragging;
        }

        void setDragging(bool dragging)
        {
            if (m_dragging != dragging)
            {
                m_dragging = dragging;
                repaintDropZoneWidgets();
            }
        }

        void registerListener(QWidget* listener)
        {
            m_listeners << listener;
        }

        void unregisterListener(QWidget* listener)
        {
            m_listeners.removeOne(listener);
        }

    private:
        void repaintDropZoneWidgets()
        {
            for (QWidget* listener : m_listeners)
            {
                listener->update();
            }
        }

        // The drop zone area mapped to the QPolygon in which we can drop QDockWidget for that zone
        QMap<Qt::DockWidgetArea, QPolygon> m_dropZones;

        // The QMainWindow or QDockWidget on which we are going to drop
        QPointer<QWidget> m_dropOnto;

        Qt::DockWidgetArea m_dropArea;

        // Used in conjunction with the above timer, the opacity of a drop zone
        // when hovered over will fade in dynamically
        qreal m_dropZoneHoverOpacity = 0.0f;

        QColor m_dropZoneColorOnHover;

        // The absolute drop zone rectangle and drop area
        bool m_onAbsoluteDropZone = false;
        QRect m_absoluteDropZoneRect;
        Qt::DockWidgetArea m_absoluteDropZoneArea;

        // The outer and inner rectangles of our current drop zones
        QRect m_dockDropZoneRect;
        QRect m_innerDropZoneRect;

        bool m_dragging = false;

        QVector<QWidget*> m_listeners;
    };

    // Splitting this out into a separate widget so that we can have one per screen
    // and so that they can be detached from any other widgets.
    // This seems to be the only reliable way to get the drop zone rendering to
    // work across multiple monitors under a number of different scenarios
    // such as multiple monitors with different scale factors and different
    // monitors selected as the primary monitor (which can matter)
    class AZ_QT_COMPONENTS_API FancyDockingDropZoneWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit FancyDockingDropZoneWidget(QMainWindow* mainWindow, QWidget* coordinatesRelativeTo, QScreen* screen, FancyDockingDropZoneState* dropZoneState);
        ~FancyDockingDropZoneWidget();

        QScreen* GetScreen();

        void Start();
        void Stop();

        static bool CheckModifierKey();

    protected:
        void paintEvent(QPaintEvent* ev) override;
        void closeEvent(QCloseEvent* ev) override;

    private:
        void paintDropZone(const Qt::DockWidgetArea area, QPolygon dropZoneShape, QPainter& painter);
        void fillAbsoluteDropZone(QPainter& painter);
        bool shouldFillAbsoluteDropZone() const;

        void paintDropBorderLines(QPainter& painter);
        bool shouldPaintDropBorderLines() const;

        QMainWindow* m_mainWindow;
        QWidget* m_relativeTo;
        QScreen* m_screen;

        FancyDockingDropZoneState* const m_dropZoneState;
    };
} // namespace AzQtComponents
