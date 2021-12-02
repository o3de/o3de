/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
                                                               // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QPainter>
AZ_POP_DISABLE_WARNING

#include "AspectRatioAwarePixmapWidget.hxx"

namespace AzToolsFramework
{
    AspectRatioAwarePixmapWidget::AspectRatioAwarePixmapWidget(QWidget* parent)
        : QWidget(parent)
        , m_pixmap()
    {
    }

    void AspectRatioAwarePixmapWidget::setPixmap(const QPixmap& p)
    {
        m_pixmap = p;

        // update the scaled, cached copy so that we recache on paint
        m_scaledPixmap = p;

        repaint();
    }
    
    int AspectRatioAwarePixmapWidget::heightForWidth(int w) const
    {
        // keep aspect ratio!
        QSize pixmapSize = m_pixmap.size();
        if (!pixmapSize.isEmpty()) // isEmpty returns true if either width or height are <= 0
        {
            float aspectRatio = static_cast<float>(pixmapSize.width()) / static_cast<float>(pixmapSize.height());
            return (int)((1.0f / aspectRatio) * (float)w);
        }
        return -1;
    }

    bool AspectRatioAwarePixmapWidget::hasHeightForWidth() const
    {
        return true;
    }


    void AspectRatioAwarePixmapWidget::paintEvent(QPaintEvent* e)
    {
        if (!m_pixmap.isNull())
        {
            QPainter p(this);

            // calculate the size to render; QSize scale with KeepAspectRatio will
            // calculate such that the new size will be as large as possible,
            // maintaining the aspect ratio and fitting within widgetSize
            QSize pixmapSize = m_pixmap.size();
            QSize widgetSize = size();
            pixmapSize.scale(widgetSize, Qt::KeepAspectRatio);

            // center it (figure out the difference and use half)
            int targetX = (size().width() - pixmapSize.width()) / 2;
            int targetY = (size().height() - pixmapSize.height()) / 2;

            QRect targetRect(QPoint(targetX, targetY), pixmapSize);

            // cache the scaled pixmap, so that drawPixmap doesn't have to do any
            // scaling
            if (m_scaledPixmap.size() != pixmapSize)
            {
                m_scaledPixmap = m_pixmap.scaled(pixmapSize, Qt::IgnoreAspectRatio);
            }

            p.drawPixmap(targetRect, m_scaledPixmap);
        }

        QWidget::paintEvent(e);
    }
}

