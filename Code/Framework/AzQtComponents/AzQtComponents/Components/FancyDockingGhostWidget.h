/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QWidget>
#include <QPixmap>

class QScreen;
class QMainWindow;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API FancyDockingGhostWidget
        : public QWidget
    {
    public:
        explicit FancyDockingGhostWidget(QMainWindow* mainWindow = nullptr, QWidget* parent = nullptr);
        ~FancyDockingGhostWidget() override;

        void setPixmap(const QPixmap& pixmap, const QRect& targetRect, QScreen* screen);

        void Enable() { m_visible = true; }
        void Disable() { m_visible = false; }

        // The equivalent of lowering the pixmap under the parent's dock widgets
        void EnableClippingToDockWidgets();

        // The equivalent of raising above all of the parent's dock widgets
        void DisableClippingToDockWidgets();

    protected:
        void closeEvent(QCloseEvent* ev) override;
        void paintEvent(QPaintEvent* ev) override;

    private:
        void setPixmapVisible(bool);
        QMainWindow* const m_mainWindow;
        QPixmap m_pixmap;
        bool m_visible = false; // maintain our own flag, so that we're always ready to render ignoring Qt's widget caching system
        bool m_clipToWidgets = false;

        //! Determines the way the ghost widget pixmap should be painted on the widget.
        enum class PaintMode
        {
            FULL = 0,       //!< Paint the pixmap on the full widget
            BOTTOMLEFT,     //!< Paint the pixmap on the bottom left quarter of the widget, halving its size
            BOTTOMRIGHT     //!< Paint the pixmap on the bottom right quarter of the widget, halving its size
        };

        PaintMode m_paintMode = PaintMode::FULL;
    };
} // namespace AzQtComponents
