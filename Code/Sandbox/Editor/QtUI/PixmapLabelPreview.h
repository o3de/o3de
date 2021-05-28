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


#ifndef PIXMAPLABELPREVIEW_H
#define PIXMAPLABELPREVIEW_H

#if !defined(Q_MOC_RUN)
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#endif

class PixmapLabelPreview
    : public QLabel
{
    Q_OBJECT
public:
    explicit PixmapLabelPreview(QWidget* parent = 0);
    virtual int heightForWidth(int width) const;
    virtual QSize sizeHint() const;

public slots:
    void setAspectRatioMode(Qt::AspectRatioMode);
    void setPixmap(const QPixmap&);
    void resizeEvent(QResizeEvent*);
private:
    QPixmap transformPixmap(QPixmap);

    QPixmap m_pixmap;
    Qt::AspectRatioMode m_mode;
};

#endif // PIXMAPLABELPREVIEW_H
