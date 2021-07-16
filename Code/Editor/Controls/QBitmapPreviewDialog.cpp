/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "QBitmapPreviewDialog.h"
#include <Controls/ui_QBitmapPreviewDialog.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>

void QBitmapPreviewDialog::ImageData::setRgba8888(const void* buffer, const int& w, const int& h)
{
    const unsigned long bytes = w * h * 4;
    m_buffer.resize(bytes);
    memcpy(m_buffer.data(), buffer, bytes);
    m_image = QImage((uchar*)m_buffer.constData(), w, h, QImage::Format::Format_RGBA8888);
}

static void fillChecker(int w, int h, unsigned int* dst)
{
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            dst[y * w + x] = 0xFF000000 | (((x >> 2) + (y >> 2)) % 2 == 0 ? 0x007F7F7F : 0x00000000);
        }
    }
}

QBitmapPreviewDialog::QBitmapPreviewDialog(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QBitmapTooltip)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Clear label text
    ui->m_placeholderBitmap->setText("");
    ui->m_placeholderHistogram->setText("");

    ui->m_bitmapSize->setProperty("tableRow", "Odd");
    ui->m_Mips->setProperty("tableRow", "Even");
    ui->m_Mean->setProperty("tableRow", "Odd");
    ui->m_StdDev->setProperty("tableRow", "Even");
    ui->m_Median->setProperty("tableRow", "Odd");
    ui->m_labelForBitmapSize->setProperty("tooltipLabel", "content");
    ui->m_labelForMean->setProperty("tooltipLabel", "content");
    ui->m_labelForMedian->setProperty("tooltipLabel", "content");
    ui->m_labelForMips->setProperty("tooltipLabel", "content");
    ui->m_labelForStdDev->setProperty("tooltipLabel", "content");
    ui->m_vBitmapSize->setProperty("tooltipLabel", "content");
    ui->m_vMean->setProperty("tooltipLabel", "content");
    ui->m_vMedian->setProperty("tooltipLabel", "content");
    ui->m_vMips->setProperty("tooltipLabel", "content");
    ui->m_vStdDev->setProperty("tooltipLabel", "content");

    // Initialize placeholder images
    const int w = 64;
    const int h = 64;
    QByteArray buffer;
    buffer.resize(w * h * 4);
    unsigned int* dst = (unsigned int*)buffer.data();
    fillChecker(w, h, dst);
    m_checker.setRgba8888(buffer.constData(), w, h);

    m_initialSize = window()->window()->geometry().size();
}

QBitmapPreviewDialog::~QBitmapPreviewDialog()
{
    delete ui;
}

void QBitmapPreviewDialog::setImageRgba8888(const void* buffer, const int& w, const int& h, [[maybe_unused]] const QString& info)
{
    m_imageMain.setRgba8888(buffer, w, h);
}


QRect QBitmapPreviewDialog::getHistogramArea()
{
    return QRect(ui->m_placeholderHistogram->pos(), ui->m_placeholderHistogram->size());
}

void QBitmapPreviewDialog::setFullSize(const bool& fullSize)
{
    if (fullSize)
    {
        QSize desktop = QApplication::screenAt(ui->m_placeholderBitmap->pos())->availableGeometry().size();
        QSize image = m_imageMain.m_image.size();
        QPoint location = mapToGlobal(ui->m_placeholderBitmap->pos());
        QSize finalSize;
        finalSize.setWidth((image.width() < (desktop.width() - location.x())) ? image.width() : (desktop.width() - location.x()));
        finalSize.setHeight((image.height() < (desktop.height() - location.y())) ? image.height() : (desktop.height() - location.y()));
        float scale = (finalSize.width() < finalSize.height()) ? finalSize.width() / float(m_imageMain.m_image.width()) : finalSize.height() / float(m_imageMain.m_image.height());
        ui->m_placeholderBitmap->setFixedSize(scale * m_imageMain.m_image.size());
    }
    else
    {
        ui->m_placeholderBitmap->setFixedSize(256, 256);
    }

    adjustSize();

    update();
}

void QBitmapPreviewDialog::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    QRect rect(ui->m_placeholderBitmap->pos(), ui->m_placeholderBitmap->size());
    drawImageData(rect, m_imageMain);
}

void QBitmapPreviewDialog::drawImageData(const QRect& rect, const ImageData& imgData)
{
    // Draw the
    QPainter p(this);
    p.drawImage(rect.topLeft(), m_checker.m_image.scaled(rect.size()));
    p.drawImage(rect.topLeft(), imgData.m_image.scaled(rect.size()));

    // Draw border
    QPen pen;
    pen.setColor(QColor(0, 0, 0));
    p.drawRect(rect.top(), rect.left(), rect.width() - 1, rect.height());
}

void QBitmapPreviewDialog::setSize(QString _value)
{
    ui->m_vBitmapSize->setText(_value);
}

void QBitmapPreviewDialog::setMips(QString _value)
{
    ui->m_vMips->setText(_value);
}

void QBitmapPreviewDialog::setMean(QString _value)
{
    ui->m_vMean->setText(_value);
}

void QBitmapPreviewDialog::setMedian(QString _value)
{
    ui->m_vMedian->setText(_value);
}

void QBitmapPreviewDialog::setStdDev(QString _value)
{
    ui->m_vStdDev->setText(_value);
}

QSize QBitmapPreviewDialog::GetCurrentBitmapSize()
{
    return ui->m_placeholderBitmap->size();
}

QSize QBitmapPreviewDialog::GetOriginalImageSize()
{
    return m_imageMain.m_image.size();
}


#include <Controls/moc_QBitmapPreviewDialog.cpp>
