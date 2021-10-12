/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Debug/TraceContext.h>

// 4251: 'QRawFont::d': class 'QExplicitlySharedDataPointer<QRawFontPrivate>' needs to have dll-interface to be used by clients of class
// 'QRawFont' 4800: 'QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <Thumbnails/ThumbnailWidget.h>
#include <UI/UICore/AspectRatioAwarePixmapWidget.hxx>
AZ_POP_DISABLE_WARNING
#include "ThumbnailPropertyCtrl.h"

namespace AzToolsFramework
{
    ThumbnailPropertyCtrl::ThumbnailPropertyCtrl(QWidget* parent)
        : QWidget(parent)
    {
        m_thumbnail = new Thumbnailer::ThumbnailWidget(this);
        m_thumbnail->setFixedSize(QSize(24, 24));

        m_thumbnailEnlarged = new Thumbnailer::ThumbnailWidget(this);
        m_thumbnailEnlarged->setFixedSize(QSize(180, 180));
        m_thumbnailEnlarged->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

        m_customThumbnail = new QLabel(this);
        m_customThumbnail->setFixedSize(QSize(24, 24));
        m_customThumbnail->setScaledContents(true);

        m_customThumbnailEnlarged = new QLabel(this);
        m_customThumbnailEnlarged->setFixedSize(QSize(180, 180));
        m_customThumbnailEnlarged->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        m_customThumbnailEnlarged->setScaledContents(true);

        m_dropDownArrow = new AspectRatioAwarePixmapWidget(this);
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0.png"));
        m_dropDownArrow->setFixedSize(QSize(8, 24));

        m_emptyThumbnail = new QLabel(this);
        m_emptyThumbnail->setPixmap(QPixmap(":/stylesheet/img/line.png"));
        m_emptyThumbnail->setFixedSize(QSize(24, 24));

        QHBoxLayout* pLayout = new QHBoxLayout();
        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(0);
        pLayout->addWidget(m_thumbnail);
        pLayout->addWidget(m_customThumbnail);
        pLayout->addWidget(m_emptyThumbnail);
        pLayout->addSpacing(4);
        pLayout->addWidget(m_dropDownArrow);
        pLayout->addSpacing(4);
        setLayout(pLayout);

        ShowDropDownArrow(false);
        UpdateVisibility();
    }

    void ThumbnailPropertyCtrl::SetThumbnailKey(Thumbnailer::SharedThumbnailKey key, const char* contextName)
    {
        if (m_customThumbnailEnabled)
        {
            ClearThumbnail();
        }
        else
        {
            m_key = key;
            m_thumbnail->SetThumbnailKey(m_key, contextName);
            m_thumbnailEnlarged->SetThumbnailKey(m_key, contextName);
        }
        UpdateVisibility();
    }

    void ThumbnailPropertyCtrl::ClearThumbnail()
    {
        m_key.clear();
        m_thumbnail->ClearThumbnail();
        m_thumbnailEnlarged->ClearThumbnail();
        UpdateVisibility();
    }

    void ThumbnailPropertyCtrl::ShowDropDownArrow(bool visible)
    {
        setFixedSize(QSize(visible ? 40 : 24, 24));
        m_dropDownArrow->setVisible(visible);
    }

    void ThumbnailPropertyCtrl::SetCustomThumbnailEnabled(bool enabled)
    {
        m_customThumbnailEnabled = enabled;
        UpdateVisibility();
    }

    void ThumbnailPropertyCtrl::SetCustomThumbnailPixmap(const QPixmap& pixmap)
    {
        m_customThumbnail->setPixmap(pixmap);
        m_customThumbnailEnlarged->setPixmap(pixmap);
        UpdateVisibility();
    }

    void ThumbnailPropertyCtrl::UpdateVisibility()
    {
        m_thumbnail->setVisible(m_key && !m_customThumbnailEnabled);
        m_thumbnailEnlarged->setVisible(false);

        m_customThumbnail->setVisible(m_customThumbnailEnabled);
        m_customThumbnailEnlarged->setVisible(false);

        m_emptyThumbnail->setVisible(!m_key && !m_customThumbnailEnabled);
    }

    bool ThumbnailPropertyCtrl::event(QEvent* e)
    {
        if (isEnabled())
        {
            if (e->type() == QEvent::MouseButtonPress)
            {
                emit clicked();
                return true; // ignore
            }
        }

        return QWidget::event(e);
    }

    void ThumbnailPropertyCtrl::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        QRect targetRect(QPoint(), QSize(40, 24));
        p.fillRect(targetRect, QColor("#111111"));
        QWidget::paintEvent(e);
    }

    void ThumbnailPropertyCtrl::enterEvent(QEvent* e)
    {
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0_highlighted.png"));
        const QPoint offset(-m_thumbnailEnlarged->width() - 5, -m_thumbnailEnlarged->height() / 2 + m_thumbnail->height() / 2);

        m_thumbnailEnlarged->move(mapToGlobal(pos()) + offset);
        m_thumbnailEnlarged->raise();
        m_thumbnailEnlarged->setVisible(m_key && !m_customThumbnailEnabled);

        m_customThumbnailEnlarged->move(mapToGlobal(pos()) + offset);
        m_customThumbnailEnlarged->raise();
        m_customThumbnailEnlarged->setVisible(m_customThumbnailEnabled);
        QWidget::enterEvent(e);
    }

    void ThumbnailPropertyCtrl::leaveEvent(QEvent* e)
    {
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0.png"));
        m_thumbnailEnlarged->setVisible(false);
        m_customThumbnailEnlarged->setVisible(false);
        QWidget::leaveEvent(e);
    }
} // namespace AzToolsFramework

#include "UI/PropertyEditor/moc_ThumbnailPropertyCtrl.cpp"
