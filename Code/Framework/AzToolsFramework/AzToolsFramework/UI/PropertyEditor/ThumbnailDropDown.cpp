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

#include <AzToolsFramework/Debug/TraceContext.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QRawFont::d': class 'QExplicitlySharedDataPointer<QRawFontPrivate>' needs to have dll-interface to be used by clients of class 'QRawFont'
                                                               // 4800: 'QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
#include <QLabel>
#include <QHBoxLayout>
#include <QEvent>
#include <QPainter>
#include <UI/UICore/AspectRatioAwarePixmapWidget.hxx>
#include <Thumbnails/ThumbnailWidget.h>
AZ_POP_DISABLE_WARNING
#include "ThumbnailDropDown.h"

namespace AzToolsFramework
{
    ThumbnailDropDown::ThumbnailDropDown(QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* pLayout = new QHBoxLayout();
        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setSpacing(0);

        m_thumbnail = new Thumbnailer::ThumbnailWidget(this);
        m_thumbnail->setFixedSize(QSize(24, 24));

        m_dropDownArrow = new AspectRatioAwarePixmapWidget(this);
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0.png"));
        m_dropDownArrow->setFixedSize(QSize(8, 24));

        m_emptyThumbnail = new QLabel(this);
        m_emptyThumbnail->setPixmap(QPixmap(":/stylesheet/img/line.png"));
        m_emptyThumbnail->setFixedSize(QSize(24, 24));

        pLayout->addWidget(m_emptyThumbnail);
        pLayout->addWidget(m_thumbnail);
        pLayout->addSpacing(4);
        pLayout->addWidget(m_dropDownArrow);
        pLayout->addSpacing(4);

        setLayout(pLayout);
    }

    void ThumbnailDropDown::SetThumbnailKey(Thumbnailer::SharedThumbnailKey key, const char* contextName)
    {
        m_emptyThumbnail->setVisible(false);
        m_thumbnail->SetThumbnailKey(key, contextName);
    }

    void ThumbnailDropDown::ClearThumbnail()
    {
        m_emptyThumbnail->setVisible(true);
        m_thumbnail->ClearThumbnail();
    }

    bool ThumbnailDropDown::event(QEvent* e)
    {
        if (isEnabled())
        {
            if (e->type() == QEvent::MouseButtonPress)
            {
                emit clicked();
                return true; //ignore
            }
        }

        return QWidget::event(e);
    }

    void ThumbnailDropDown::paintEvent(QPaintEvent* e)
    {
        QPainter p(this);
        QRect targetRect(QPoint(), QSize(40, 24));
        p.fillRect(targetRect, QColor(17, 17, 17)); // #111111
        QWidget::paintEvent(e);
    }

    void ThumbnailDropDown::enterEvent(QEvent* e)
    {
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0_highlighted.png"));
        QWidget::enterEvent(e);
    }

    void ThumbnailDropDown::leaveEvent(QEvent* e)
    {
        m_dropDownArrow->setPixmap(QPixmap(":/stylesheet/img/triangle0.png"));
        QWidget::leaveEvent(e);
    }
}

#include "UI/PropertyEditor/moc_ThumbnailDropDown.cpp"
