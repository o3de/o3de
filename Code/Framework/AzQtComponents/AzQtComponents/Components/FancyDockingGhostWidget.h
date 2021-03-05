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
    };
} // namespace AzQtComponents