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

#include <AzCore/PlatformDef.h>

#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

// 4127: conditional expression is constant
// 4800 'uint': forcing value to bool 'true' or 'false' (performance warning)
// 4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
AZ_PUSH_DISABLE_WARNING(4127 4800 4251, "-Wunknown-warning-option") 
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>
AZ_POP_DISABLE_WARNING


namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailWidget::ThumbnailWidget(QWidget* parent)
            : QWidget(parent)
        {
        }

        void ThumbnailWidget::SetThumbnailKey(SharedThumbnailKey key, const char* contextName)
        {
            if (m_key)
            {
                disconnect(m_key.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &ThumbnailWidget::KeyUpdatedSlot);
            }
            m_key = key;
            m_contextName = contextName;
            connect(m_key.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &ThumbnailWidget::KeyUpdatedSlot);
            repaint();
        }

        void ThumbnailWidget::ClearThumbnail()
        {
            m_key.clear();
            repaint();
        }

        int ThumbnailWidget::heightForWidth(int w) const
        {
            return w;
        }

        QSize ThumbnailWidget::sizeHint() const
        {
            int w = this->width();
            return QSize(w, heightForWidth(w));
        }

        void ThumbnailWidget::paintEvent(QPaintEvent* event)
        {
            if (m_key)
            {
                // thumbnail instance is not stored locally, but retrieved each paintEvent since thumbnail mapped to a specific key may change
                SharedThumbnail thumbnail;
                ThumbnailerRequestsBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, m_key, m_contextName.c_str());
                QPainter painter(this);
                QPixmap pixmap = thumbnail->GetPixmap();
                // preserve thumbnail image's ratio, if the widget is wider than the image, center the image hotizontally
                float aspectRatio = aznumeric_cast<float>(pixmap.width()) / pixmap.height();
                int originalWidth = width();
                int originalHeight = height();
                int realHeight = qMin(aznumeric_cast<int>(originalWidth /aspectRatio), originalHeight);
                int realWidth = aznumeric_cast<int>(realHeight * aspectRatio);
                int x = (originalWidth - realWidth) / 2;
                // pixmap needs to be manually scaled to produce smoother result and avoid looking pixelated
                // using painter.setRenderHint(QPainter::SmoothPixmapTransform); does not seem to work
                painter.drawPixmap(QPoint(x, 0), pixmap.scaled(realWidth, realHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
            QWidget::paintEvent(event);
        }

        void ThumbnailWidget::KeyUpdatedSlot()
        {
            repaint();
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_ThumbnailWidget.cpp"
