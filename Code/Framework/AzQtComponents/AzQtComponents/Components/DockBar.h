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

#include <QColor>
#include <QObject>
#include <QPixmap>
#include <QString>
#endif

class QPainter;
class QRect;

namespace AzQtComponents
{
    struct DockBarColors
    {
        QColor text;
        QColor frame;
        QColor firstLine;
        QColor gradientStart;
        QColor gradientEnd;
    };

    class AZ_QT_COMPONENTS_API DockBar
        : public QObject
    {
        Q_OBJECT
    public:
        enum
        {
            Height = 32,
            HandleLeftMargin = 3,
            TitleLeftMargin = 8,
            TitleRightMargin = 18,
            CloseButtonRightMargin = 2,
            ButtonsSpacing = 5,
            ResizeTopMargin = 4,
            MaxTabTitleWidth = 200
        };

        static void drawFrame(QPainter* painter, const QRect& area, bool drawSideBorders, const DockBarColors& colors);
        static void drawTabContents(QPainter* painter, const QRect& area, const DockBarColors& colors, const QString& title);
        static QString GetTabTitleElided(const QString& title, int& textWidth);
        static int GetTabTitleMinWidth(const QString& title, bool enableTear = true);
        static DockBarColors getColors(bool active);

        explicit DockBar(QObject* parent = nullptr);
        DockBarColors GetColors(bool active) { return DockBar::getColors(active); }

    private:
        static int drawIcon(QPainter* painter, int x, const QPixmap& icon);
        static void drawTabTitle(QPainter* painter, int leftContentWidth, const QRect& area,
            int buttonsX, const QColor& color, const QString& title);
        QPixmap m_tearIcon;
        QPixmap m_applicationIcon;
    };
} // namespace AzQtComponents
