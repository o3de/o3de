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

    return ((qreal)m_pixmap.height() * width) / m_pixmap.width();
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
