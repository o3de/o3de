/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
