/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef QBITMAPPREVIEWDIALOG_H
#define QBITMAPPREVIEWDIALOG_H

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QPixmap>
#include <QImage>
#endif

class QLabel;

namespace Ui {
    class QBitmapTooltip;
}

class QBitmapPreviewDialog
    : public QWidget
{
    Q_OBJECT

    struct ImageData
    {
        QByteArray  m_buffer;
        QImage      m_image;

        void setRgba8888(const void* buffer, const int& w, const int& h);
    };

public:
    explicit QBitmapPreviewDialog(QWidget* parent = 0);
    virtual ~QBitmapPreviewDialog();
    QSize GetCurrentBitmapSize();
    QSize GetOriginalImageSize();

protected:
    void setImageRgba8888(const void* buffer, const int& w, const int& h, const QString& info);
    void setSize(QString _value);
    void setMips(QString _value);
    void setMean(QString _value);
    void setMedian(QString _value);
    void setStdDev(QString _value);
    QRect getHistogramArea();
    void setFullSize(const bool& fullSize);

    void paintEvent(QPaintEvent* e) override;

private:
    void drawImageData(const QRect& rect, const ImageData& imgData);

protected:
    Ui::QBitmapTooltip* ui;
    QSize       m_initialSize;
    ImageData   m_checker;
    ImageData   m_imageMain;
};

#endif // QBITMAPPREVIEWDIALOG_H
