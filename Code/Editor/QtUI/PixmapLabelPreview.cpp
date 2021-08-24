/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PixmapLabelPreview.h"

PixmapLabelPreview::PixmapLabelPreview(QWidget* parent)
    : QLabel(parent)
    , m_mode(Qt::IgnoreAspectRatio)
{
    this->setMinimumSize(10, 10);
}

void PixmapLabelPreview::setPixmap(const QPixmap& p)
{
    m_pixmap = p;
    QLabel::setPixmap(transformPixmap(p));
}

int PixmapLabelPreview::heightForWidth(int width) const
{
    if (m_mode == Qt::KeepAspectRatio)
    {
        return width;
    }

    return static_cast<int>(((qreal)m_pixmap.height() * width) / m_pixmap.width());
}


QSize PixmapLabelPreview::sizeHint() const
{
    int w = this->width();
    return QSize(w, heightForWidth(w));
}

void PixmapLabelPreview::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_mode = mode;
}

void PixmapLabelPreview::resizeEvent([[maybe_unused]] QResizeEvent* e)
{
    QLabel::setPixmap(transformPixmap(m_pixmap));
}

QPixmap PixmapLabelPreview::transformPixmap(QPixmap pix)
{
    return pix.scaled(this->size(), m_mode, Qt::SmoothTransformation);
}

#include <QtUI/moc_PixmapLabelPreview.cpp>
