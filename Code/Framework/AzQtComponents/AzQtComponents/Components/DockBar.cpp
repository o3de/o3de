/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/DockBar.h>

#include <QLinearGradient>
#include <QPainter>
#include <QRect>
#include <QFont>
#include <QFontMetrics>
#include <QPixmapCache>

// Constant for the dock bar text font family
static const char* g_dockBarFontFamily = "Open Sans";
// Constant for the dock bar text point size
static const int g_dockBarFontPointSize = 8;
// Constant for application icon path
static const char* g_applicationIconPath = ":/stylesheet/img/ly_application_icon.png";
// Constant for dock bar tear handle icon path
static const char* g_dockBarTearIconPath = ":/stylesheet/img/titlebar_tear.png";


namespace AzQtComponents
{
    namespace
    {
        QPixmap tearIcon()
        {
            QPixmap tearIcon;
            if (!QPixmapCache::find(QStringLiteral("dockBarTearIcon"), &tearIcon))
            {
                tearIcon.load(g_dockBarTearIconPath);
                QPixmapCache::insert(QStringLiteral("dockBarTearIcon"), tearIcon);
            }
            return tearIcon;
        }
    } // namespace

    void DockBar::drawFrame(QPainter* painter, const QRect& area, bool drawSideBorders, const DockBarColors& colors)
    {
        painter->save();
        // Top row
        painter->setPen(colors.firstLine);
        painter->drawLine(0, 1, area.right(), 1);

        // Background
        QLinearGradient background(area.topLeft(), area.bottomLeft());
        background.setColorAt(0, colors.gradientStart);
        background.setColorAt(1, colors.gradientEnd);
        painter->fillRect(area.adjusted(0, 2, 0, -1), background);

        // Frame
        painter->setPen(colors.frame);
        painter->drawLine(0, area.top(), area.right(), area.top()); // top
        painter->drawLine(0, area.bottom(), area.right(), area.bottom()); // bottom

        // We can't draw left and right border here, because the titlebar is not as wide as the
        // dockwidget, because Qt internally sets the title bar width to dockwidget.width() - 2 * frame.
        // Setting frame to 0 would fix it, but then we wouldn't have border/shadow.
        // We draw these two lines inside StyledDockWidget::paintEvent() instead.
        if (drawSideBorders)
        {
            painter->drawLine(0, 0, 0, area.height() - 1); // left
            painter->drawLine(area.right(), 0, area.right(), area.height() - 1); // right
        }
        painter->restore();
    }

    void DockBar::drawTabContents(QPainter* painter, const QRect& area, const DockBarColors& colors, const QString& title)
    {
        painter->save();

        // Draw either the tear icon or the application icon
        const int iconWidth = drawIcon(painter, area.x(), tearIcon());
        // Draw the title using the icon width as the left margin
        drawTabTitle(painter, iconWidth, area, area.right(), colors.text, title);

        painter->restore();
    }

    QString DockBar::GetTabTitleElided(const QString& title, int& textWidth)
    {
        const QFontMetrics fontMetrics({ g_dockBarFontFamily, g_dockBarFontPointSize });

        textWidth = fontMetrics.horizontalAdvance(title);
        if (textWidth > MaxTabTitleWidth)
        {
            textWidth = MaxTabTitleWidth;
        }

        return fontMetrics.elidedText(title, Qt::ElideRight, textWidth);
    }

    /**
     * Return the minimum width in pixels of a dock bar based on the title width plus all the margin offsets
     */
    int DockBar::GetTabTitleMinWidth(const QString& title, bool enableTear)
    {
        // Calculate the base width of the text (capped at our max title width) plus margins
        int textWidth = 0;
        GetTabTitleElided(title, textWidth);
        int width = HandleLeftMargin + TitleLeftMargin + textWidth + TitleRightMargin + ButtonsSpacing;

        // If we have enabled tearing, add in the width of the tear icon
        if (enableTear)
        {
            width += tearIcon().width();
        }

        return width;
    }

    /**
     * Return the appropriate DockBarColors struct based on if it is active or not
     */
    DockBarColors DockBar::getColors(bool active)
    {
        if (active)
        {
            return {
                {
                    204, 204, 204
                },{
                    33, 34, 35
                },{
                    64, 68, 69
                },{
                    64, 72, 80
                },{
                    54, 61, 68
                }
            };
        }
        else
        {
            return {
                {
                    204, 204, 204
                },{
                    33, 34, 35
                },{
                    64, 68, 69
                },{
                    65, 68, 69
                },{
                    54, 56, 57
                }
            };
        }
    }

    /**
     * Create a dock tab widget that extends a QTabWidget with a custom DockTabBar to replace the default tab bar
     */
    DockBar::DockBar(QObject* parent)
        : QObject(parent)
        , m_tearIcon(g_dockBarTearIconPath)
        , m_applicationIcon(g_applicationIconPath)
    {
    }

    /**
     * Draw the specified icon and return its width
     */
    int DockBar::drawIcon(QPainter* painter, int x, const QPixmap& icon)
    {
        painter->drawPixmap(QPointF(HandleLeftMargin + x, Height / 2 - icon.height() / 2), icon, icon.rect());
        return icon.width();
    }

    /**
     * Draw the specified title on our dock tab bar
     */
    void DockBar::drawTabTitle(QPainter* painter, int leftContentWidth, const QRect& area,
        int buttonsX, const QColor& color, const QString& title)
    {
        if (title.isEmpty())
        {
            return;
        }

        const int textX = HandleLeftMargin + leftContentWidth + TitleLeftMargin + area.x();
        const int maxX = buttonsX - TitleRightMargin;
        QFont f(painter->font());
        f.setFamily(g_dockBarFontFamily);
        f.setPointSize(g_dockBarFontPointSize);
        painter->setFont(f);
        painter->setPen(color);

        // Cap our maximum allowed tab title width
        int textWidth = maxX - textX;

        // Elide our title text if it exceeds the maximum width
        QFontMetrics fontMetrics = painter->fontMetrics();
        QString elidedTitle = fontMetrics.elidedText(title, Qt::ElideRight, textWidth);

        // We use the Qt::TextSingleLine flag to make sure whitespace is all treated
        // as spaces so that the text all prints on a single line, otherwise it would
        // try to render the text on multiple lines even if you restrict the height
        painter->drawText(QRectF(textX, 0, textWidth, area.height() - 1), Qt::AlignVCenter | Qt::TextSingleLine, elidedTitle);
    }

} // namespace AzQtComponents

#include "Components/moc_DockBar.cpp"
