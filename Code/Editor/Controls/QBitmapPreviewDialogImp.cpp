/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "QBitmapPreviewDialogImp.h"

// Cry
#include <ITexture.h>

// EditorCore
#include <Util/Image.h>
#include <Include/IImageUtil.h>

// QT
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
#include <QEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <qmath.h>
AZ_POP_DISABLE_WARNING

#include <Controls/ui_QBitmapPreviewDialog.h>

static const int kDefaultWidth  = 256;
static const int kDefaultHeight = 256;

QBitmapPreviewDialogImp::QBitmapPreviewDialogImp(QWidget* parent)
    : QBitmapPreviewDialog(parent)
    , m_image(new CImageEx())
    , m_showOriginalSize(false)
    , m_showMode(ESHOW_RGB)
    , m_histrogramMode(eHistogramMode_OverlappedRGB)
{
    setMouseTracking(true);
    setImage("");
    ui->m_placeholderBitmap->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
    ui->m_placeholderHistogram->setStyleSheet("background-color: rgba(0, 0, 0, 0);");

    ui->m_labelForBitmapSize->setProperty("tooltipLabel", "Content");
    ui->m_labelForMean->setProperty("tooltipLabel", "Content");
    ui->m_labelForMedian->setProperty("tooltipLabel", "Content");
    ui->m_labelForMips->setProperty("tooltipLabel", "Content");
    ui->m_labelForStdDev->setProperty("tooltipLabel", "Content");

    ui->m_vBitmapSize->setProperty("tooltipLabel", "Content");
    ui->m_vMean->setProperty("tooltipLabel", "Content");
    ui->m_vMedian->setProperty("tooltipLabel", "Content");
    ui->m_vMips->setProperty("tooltipLabel", "Content");
    ui->m_vStdDev->setProperty("tooltipLabel", "Content");

    setUIStyleMode(EUISTYLE_IMAGE_ONLY);
}

QBitmapPreviewDialogImp::~QBitmapPreviewDialogImp()
{
    SAFE_DELETE(m_image);
}

void QBitmapPreviewDialogImp::setImage(const QString path)
{
    if (path.isEmpty()
        ||  m_path == path
        ||  !GetIEditor()->GetImageUtil()->LoadImage(path.toUtf8().data(), *m_image))
    {
        return;
    }
    
    m_showOriginalSize = isSizeSmallerThanDefault();
    m_path = path;
    refreshData();
}

void QBitmapPreviewDialogImp::setShowMode(EShowMode mode)
{
    if (mode == ESHOW_NumModes)
    {
        return;
    }

    m_showMode = mode;
    refreshData();
    update();
}

void QBitmapPreviewDialogImp::toggleShowMode()
{
    m_showMode = (EShowMode)(((int)m_showMode + 1) % ESHOW_NumModes);
    refreshData();
    update();
}

void QBitmapPreviewDialogImp::setUIStyleMode(EUIStyle mode)
{
    if (mode >= EUISTYLE_NumModes)
    {
        return;
    }

    m_uiStyle = mode;
    if (m_uiStyle == EUISTYLE_IMAGE_ONLY)
    {
        ui->m_placeholderHistogram->hide();

        ui->m_labelForBitmapSize->hide();
        ui->m_labelForMean->hide();
        ui->m_labelForMedian->hide();
        ui->m_labelForMips->hide();
        ui->m_labelForStdDev->hide();

        ui->m_vBitmapSize->hide();
        ui->m_vMean->hide();
        ui->m_vMedian->hide();
        ui->m_vMips->hide();
        ui->m_vStdDev->hide();
    }
    else
    {
        ui->m_placeholderHistogram->show();

        ui->m_labelForBitmapSize->show();
        ui->m_labelForMean->show();
        ui->m_labelForMedian->show();
        ui->m_labelForMips->show();
        ui->m_labelForStdDev->show();

        ui->m_vBitmapSize->show();
        ui->m_vMean->show();
        ui->m_vMedian->show();
        ui->m_vMips->show();
        ui->m_vStdDev->show();
    }
}

const QBitmapPreviewDialogImp::EShowMode& QBitmapPreviewDialogImp::getShowMode() const
{
    return m_showMode;
}

void QBitmapPreviewDialogImp::setHistogramMode(EHistogramMode mode)
{
    if (mode == eHistogramMode_NumModes)
    {
        return;
    }

    m_histrogramMode = mode;
}

void QBitmapPreviewDialogImp::toggleHistrogramMode()
{
    m_histrogramMode = (EHistogramMode)(((int)m_histrogramMode + 1) % eHistogramMode_NumModes);
    update();
}

const QBitmapPreviewDialogImp::EHistogramMode& QBitmapPreviewDialogImp::getHistogramMode() const
{
    return m_histrogramMode;
}

void QBitmapPreviewDialogImp::toggleOriginalSize()
{
    m_showOriginalSize = !m_showOriginalSize;

    refreshData();
    update();
}

bool QBitmapPreviewDialogImp::isSizeSmallerThanDefault()
{
    return m_image->GetWidth() < kDefaultWidth && m_image->GetHeight() < kDefaultHeight;
}

void QBitmapPreviewDialogImp::setOriginalSize(bool value)
{
    m_showOriginalSize = value;

    refreshData();
    update();
}


const char* QBitmapPreviewDialogImp::GetShowModeDescription(EShowMode eShowMode, [[maybe_unused]] bool bShowInOriginalSize) const
{
    switch (eShowMode)
    {
    case ESHOW_RGB:
        return "RGB";
    case ESHOW_RGB_ALPHA:
        return "RGB+A";
    case ESHOW_ALPHA:
        return "Alpha";
    case ESHOW_RGBA:
        return "RGBA";
    case ESHOW_RGBE:
        return "RGBExp";
    }

    return "";
}

const char* getHistrogramModeStr(QBitmapPreviewDialogImp::EHistogramMode mode, bool shortName)
{
    switch (mode)
    {
    case QBitmapPreviewDialogImp::eHistogramMode_Luminosity:
        return shortName ? "Lum" : "Luminosity";
    case QBitmapPreviewDialogImp::eHistogramMode_OverlappedRGB:
        return shortName ? "Overlap" : "Overlapped RGBA";
    case QBitmapPreviewDialogImp::eHistogramMode_SplitRGB:
        return shortName ? "R|G|B" : "Split RGB";
    case QBitmapPreviewDialogImp::eHistogramMode_RedChannel:
        return shortName ? "Red" : "Red Channel";
    case QBitmapPreviewDialogImp::eHistogramMode_GreenChannel:
        return shortName ? "Green" : "Green Channel";
    case QBitmapPreviewDialogImp::eHistogramMode_BlueChannel:
        return shortName ? "Blue" : "Blue Channel";
    case QBitmapPreviewDialogImp::eHistogramMode_AlphaChannel:
        return shortName ? "Alpha" : "Alpha Channel";
    default:
        break;
    }

    return "";
}

void QBitmapPreviewDialogImp::refreshData()
{
    // Check if we have some usefull data loaded
    if (m_image->GetWidth() * m_image->GetHeight() == 0)
    {
        return;
    }

    int w = m_image->GetWidth();
    int h = m_image->GetHeight();

    int multiplier = (m_showMode == ESHOW_RGB_ALPHA ? 2 : 1);
    int originalW = w * multiplier;
    int originalH = h;

    if (!m_showOriginalSize || (w == 0))
    {
        w = kDefaultWidth;
    }
    if (!m_showOriginalSize || (h == 0))
    {
        h = kDefaultHeight;
    }

    w *= multiplier;

    CImageEx scaledImage;

    if (m_showOriginalSize && (originalW < w))
    {
        w = originalW;
    }
    if (m_showOriginalSize && (originalH < h))
    {
        h = originalH;
    }

    scaledImage.Allocate(w, h);

    if (m_showMode == ESHOW_RGB_ALPHA)
    {
        GetIEditor()->GetImageUtil()->ScaleToDoubleFit(*m_image, scaledImage);
    }
    else
    {
        GetIEditor()->GetImageUtil()->ScaleToFit(*m_image, scaledImage);
    }

    if (m_showMode == ESHOW_RGB || m_showMode == ESHOW_RGBE)
    {
        scaledImage.FillAlpha();
    }
    else if (m_showMode == ESHOW_ALPHA)
    {
        for (int h2 = 0; h2 < scaledImage.GetHeight(); h2++)
        {
            for (int w2 = 0; w2 < scaledImage.GetWidth(); w2++)
            {
                int a = scaledImage.ValueAt(w2, h2) >> 24;
                scaledImage.ValueAt(w2, h2) = RGB(a, a, a) | (a << 24);
            }
        }
    }
    else if (m_showMode == ESHOW_RGB_ALPHA)
    {
        int halfWidth = scaledImage.GetWidth() / 2;
        for (int h2 = 0; h2 < scaledImage.GetHeight(); h2++)
        {
            for (int w2 = 0; w2 < halfWidth; w2++)
            {
                int r = GetRValue(scaledImage.ValueAt(w2, h2));
                int g = GetGValue(scaledImage.ValueAt(w2, h2));
                int b = GetBValue(scaledImage.ValueAt(w2, h2));
                int a = scaledImage.ValueAt(w2, h2) >> 24;
                scaledImage.ValueAt(w2, h2) = RGB(r, g, b) | (a << 24);
                scaledImage.ValueAt(w2 + halfWidth, h2) = RGB(a, a, a) | (a << 24);
            }
        }
    }


    setImageRgba8888(scaledImage.GetData(), w, h, "");
    setSize(QString().asprintf("%d x %d", m_image->GetWidth(), m_image->GetHeight()));
    setMips(QString().asprintf("%d", m_image->GetNumberOfMipMaps()));

    setFullSize(m_showOriginalSize);

    // Compute histogram
    m_histogram.ComputeHistogram((BYTE*)scaledImage.GetData(), w, h, CImageHistogram::eImageFormat_32BPP_RGBA);
}

void QBitmapPreviewDialogImp::paintEvent(QPaintEvent* e)
{
    QBitmapPreviewDialog::paintEvent(e);

    //if showing original size hide other information so it's easier to see
    if (m_showOriginalSize)
    {
        return;
    }
    if (m_uiStyle == EUISTYLE_IMAGE_ONLY)
    {
        return;
    }

    QPainter p(this);
    QPen pen;
    QPainterPath path[4];

    // Fill background color
    QRect histogramRect = getHistogramArea();
    p.fillRect(histogramRect, QColor(255, 255, 255));

    // Draw borders
    pen.setColor(QColor(0, 0, 0));
    p.setPen(pen);
    p.drawRect(histogramRect);

    // Draw histogram

    QVector<int> drawChannels;

    switch (m_histrogramMode)
    {
    case eHistogramMode_Luminosity:
        drawChannels.push_back(3);
        break;
    case eHistogramMode_SplitRGB:
        drawChannels.push_back(0);
        drawChannels.push_back(1);
        drawChannels.push_back(2);
        break;
    case eHistogramMode_OverlappedRGB:
        drawChannels.push_back(0);
        drawChannels.push_back(1);
        drawChannels.push_back(2);
        break;
    case eHistogramMode_RedChannel:
        drawChannels.push_back(0);
        break;
    case eHistogramMode_GreenChannel:
        drawChannels.push_back(1);
        break;
    case eHistogramMode_BlueChannel:
        drawChannels.push_back(2);
        break;
    case eHistogramMode_AlphaChannel:
        drawChannels.push_back(3);
        break;
    }

    int graphWidth =  qMax(histogramRect.width(), 1);
    int graphHeight = qMax(histogramRect.height() - 2, 0);
    int graphBottom = histogramRect.bottom() + 1;
    int currX[4] = {0, 0, 0, 0};
    int prevX[4] = {0, 0, 0, 0};
    float scale = 0.0f;
    static const int numSubGraphs = 3;
    const int subGraph = qCeil(graphWidth / numSubGraphs);

    // Fill background for Split RGB histogram
    if (m_histrogramMode == eHistogramMode_SplitRGB)
    {
        const static QColor backgroundColor[numSubGraphs] =
        {
            QColor(255, 220, 220),
            QColor(220, 255, 220),
            QColor(220, 220, 255)
        };

        for (int i = 0; i < numSubGraphs; i++)
        {
            p.fillRect(histogramRect.left() + subGraph * i,
                histogramRect.top(),
                subGraph + (i == numSubGraphs - 1 ? 1 : 0),
                histogramRect.height(), backgroundColor[i]);
        }
    }

    int lastHeight[CImageHistogram::kNumChannels] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };

    for (int x = 0; x < graphWidth; ++x)
    {
        for (int j = 0; j < drawChannels.size(); j++)
        {
            const int c = drawChannels[j];
            int& curr_x = currX[c];
            int& prev_x = prevX[c];
            int& last_height = lastHeight[c];
            QPainterPath& curr_path = path[c];


            curr_x = histogramRect.left() + x + 1;
            int i = ((float)x / (graphWidth - 1)) * (CImageHistogram::kNumColorLevels - 1);
            if (m_histrogramMode == eHistogramMode_SplitRGB)
            {
                // Filter out to area which we are interested
                const int k = x / subGraph;
                if (k != c)
                {
                    continue;
                }

                i = qCeil((i - (subGraph * c)) * numSubGraphs);
                i = qMin(i, CImageHistogram::kNumColorLevels - 1);
                i = qMax(i, 0);
            }

            if (m_histrogramMode == eHistogramMode_Luminosity)
            {
                scale = (float)m_histogram.m_lumCount[i] / m_histogram.m_maxLumCount;
            }
            else if (m_histogram.m_maxCount[c])
            {
                scale = (float)m_histogram.m_count[c][i] / m_histogram.m_maxCount[c];
            }

            int height = graphBottom - graphHeight * scale;
            if (last_height == INT_MAX)
            {
                last_height = height;
            }

            curr_path.moveTo(prev_x, last_height);
            curr_path.lineTo(curr_x, height);
            last_height = height;

            if (prev_x == INT_MAX)
            {
                prev_x = curr_x;
            }

            prev_x = curr_x;
        }
    }

    static const QColor kChannelColor[4] =
    {
        QColor(255, 0, 0),
        QColor(0, 255, 0),
        QColor(0, 0, 255),
        QColor(120, 120, 120)
    };

    for (int i = 0; i < drawChannels.size(); i++)
    {
        const int c = drawChannels[i];
        pen.setColor(kChannelColor[c]);
        p.setPen(pen);
        p.drawPath(path[c]);
    }

    // Update histogram info
    {
        float mean = 0, stdDev = 0, median = 0;

        switch (m_histrogramMode)
        {
        case eHistogramMode_Luminosity:
        case eHistogramMode_SplitRGB:
        case eHistogramMode_OverlappedRGB:
            mean = m_histogram.m_meanAvg;
            stdDev = m_histogram.m_stdDevAvg;
            median = m_histogram.m_medianAvg;
            break;
        case eHistogramMode_RedChannel:
            mean = m_histogram.m_mean[0];
            stdDev = m_histogram.m_stdDev[0];
            median = m_histogram.m_median[0];
            break;
        case eHistogramMode_GreenChannel:
            mean = m_histogram.m_mean[1];
            stdDev = m_histogram.m_stdDev[1];
            median = m_histogram.m_median[1];
            break;
        case eHistogramMode_BlueChannel:
            mean = m_histogram.m_mean[2];
            stdDev = m_histogram.m_stdDev[2];
            median = m_histogram.m_median[2];
            break;
        case eHistogramMode_AlphaChannel:
            mean = m_histogram.m_mean[3];
            stdDev = m_histogram.m_stdDev[3];
            median = m_histogram.m_median[3];
            break;
        }
        QString val;
        val.setNum(mean);
        setMean(val);
        val.setNum(stdDev);
        setStdDev(val);
        val.setNum(median);
        setMedian(val);
    }
}

#include <Controls/moc_QBitmapPreviewDialogImp.cpp>
