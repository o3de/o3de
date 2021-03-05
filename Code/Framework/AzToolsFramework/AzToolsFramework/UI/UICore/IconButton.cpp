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

#include "IconButton.hxx"

#include <AzCore/Casting/numeric_cast.h>

#include <QEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>

namespace AzToolsFramework
{
    void IconButton::enterEvent(QEvent *event)
    {
        // do not update the button if it is disabled
        if (!isEnabled())
        {
            m_mouseOver = false;
            return;
        }

        m_mouseOver = true;
        QPushButton::enterEvent(event);
    }

    void IconButton::leaveEvent(QEvent *event)
    {
        m_mouseOver = false;
        QPushButton::enterEvent(event);
    }

    void IconButton::paintEvent(QPaintEvent* /*event*/)
    {
        if (m_currentIconCacheKey != icon().cacheKey() + highlightedIcon().cacheKey() || m_currentIconCacheKeySize != size())
        {
            m_currentIconCacheKey = icon().cacheKey() + highlightedIcon().cacheKey();
            m_currentIconCacheKeySize = size();

            // We want the pixmap of the largest size that's smaller than us.
            m_iconPixmap = icon().pixmap(size());

            if (!highlightedIcon().isNull())
            {
                m_highlightedIconPixmap = highlightedIcon().pixmap(size());
            }
            else
            {
                m_highlightedIconPixmap = m_iconPixmap;
                QPainter pixmapPainter;
                pixmapPainter.begin(&m_highlightedIconPixmap);
                pixmapPainter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                pixmapPainter.fillRect(m_highlightedIconPixmap.rect(), QColor(250, 250, 250, 180));
                pixmapPainter.end();
            }
        }

        QPainter painter(this);

        /*
         * Fit the icon image into the center of the EntityIconButton
         */
        QRect destRect = m_iconPixmap.rect();
        if (destRect.width() > width())
        {
            float rel = static_cast<float>(width()) / static_cast<float>(destRect.width());
            destRect.setSize(destRect.size() * rel);
        }
        if (destRect.height() > height())
        {
            float rel = static_cast<float>(height()) / static_cast<float>(destRect.height());
            destRect.setSize(destRect.size() * rel);
        }
        destRect.moveCenter(rect().center());

        if (m_mouseOver)
        {
            painter.setCompositionMode(QPainter::CompositionMode_Overlay);
            painter.drawPixmap(destRect, m_highlightedIconPixmap, m_highlightedIconPixmap.rect());
        }
        else
        {
            painter.drawPixmap(destRect, m_iconPixmap, m_iconPixmap.rect());
        }
    }
}
