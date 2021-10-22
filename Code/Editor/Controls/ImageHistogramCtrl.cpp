/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ImageHistogramCtrl.h"

// Qt
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>


// CImageHistogramCtrl

// control tweak constants
namespace ImageHistogram
{
    const float     kGraphHeightPercent = 0.7f;
    const int       kGraphMargin = 4;
    const QColor    kBackColor = QColor(100, 100, 100);
    const QColor    kRedSectionColor = QColor(255, 220, 220);
    const QColor    kGreenSectionColor = QColor(220, 255, 220);
    const QColor    kBlueSectionColor = QColor(220, 220, 255);
    const QColor    kSplitSeparatorColor = QColor(100, 100, 0);
    const int       kTextFontSize = 70;
    const char*     kTextFontFace = "Arial";
    const QColor    kTextColor(255, 255, 255);
};

CImageHistogramCtrl::CImageHistogramCtrl(QWidget* parent)
    : QWidget(parent)
    , m_display(new CImageHistogramDisplay(this))
    , m_drawMode(new QComboBox(this))
    , m_infoText(new QLabel)
{
    setWindowTitle("Image Histogram");

    m_drawMode->setFocusPolicy(Qt::NoFocus);
    m_drawMode->addItem(tr("Luminosity"),
                        QVariant::fromValue(EHistogramDrawMode::Luminosity));
    m_drawMode->addItem(tr("Overlapped RGBA"),
                        QVariant::fromValue(EHistogramDrawMode::OverlappedRGB));
    m_drawMode->addItem(tr("Split RGB"),
                        QVariant::fromValue(EHistogramDrawMode::SplitRGB));
    m_drawMode->addItem(tr("Red Channel"),
                        QVariant::fromValue(EHistogramDrawMode::RedChannel));
    m_drawMode->addItem(tr("Green Channel"),
                        QVariant::fromValue(EHistogramDrawMode::GreenChannel));
    m_drawMode->addItem(tr("Blue Channel"),
                        QVariant::fromValue(EHistogramDrawMode::BlueChannel));
    m_drawMode->addItem(tr("Alpha Channel"),
                        QVariant::fromValue(EHistogramDrawMode::AlphaChannel));

    connect(m_drawMode, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [=]([[maybe_unused]] int index){
                auto mode = drawMode();
                m_display->m_drawMode = mode;
                m_display->update();
            });

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_drawMode);
    layout->addWidget(m_display);
    setLayout(layout);

    setMinimumSize(200,150);
}

CImageHistogramCtrl::~CImageHistogramCtrl()
{
}

EHistogramDrawMode CImageHistogramCtrl::drawMode() const
{
    return m_drawMode->currentData().value<EHistogramDrawMode>();
}

void CImageHistogramCtrl::setDrawMode(EHistogramDrawMode mode)
{
    auto index = m_drawMode->findData(QVariant::fromValue(mode));
    if (index != -1)
    {
        m_drawMode->setCurrentIndex(index);
    }
}

void CImageHistogramCtrl::ComputeHistogram(CImageEx& image, CImageHistogram::EImageFormat format)
{
    m_display->ComputeHistogram((BYTE*)image.GetData(), image.GetWidth(), image.GetHeight(), format);
}


CImageHistogramDisplay::CImageHistogramDisplay(QWidget* parent)
    : QWidget(parent)
{
    m_graphMargin = ImageHistogram::kGraphMargin;
    m_drawMode = EHistogramDrawMode::Luminosity;
    m_backColor = ImageHistogram::kBackColor;
    m_graphHeightPercent = ImageHistogram::kGraphHeightPercent;
}

CImageHistogramDisplay::~CImageHistogramDisplay()
{}

void CImageHistogramDisplay::paintEvent([[maybe_unused]] QPaintEvent* event)
{
    QPainter painter(this);

    QColor   penColor;
    QString  str, mode;
    QRect    rc, rcGraph;
    QPen     penSpikes;
    QFont    fnt;
    QPen     redPen, greenPen, bluePen, alphaPen;

    redPen = QColor(255, 0, 0);
    greenPen = QColor(0, 255, 0);
    bluePen = QColor(0, 0, 255);
    alphaPen = QColor(120, 120, 120);

    rc = rect();

    if (rc.isEmpty())
    {
        return;
    }

    painter.fillRect(rc, m_backColor);
    penColor = QColor(0, 0, 0);

    switch (m_drawMode)
    {
    case EHistogramDrawMode::Luminosity:
        mode = "Lum";
        break;
    case EHistogramDrawMode::OverlappedRGB:
        mode = "Overlap";
        break;
    case EHistogramDrawMode::SplitRGB:
        mode = "R|G|B";
        break;
    case EHistogramDrawMode::RedChannel:
        mode = "Red";
        penColor = QColor(255, 0, 0);
        break;
    case EHistogramDrawMode::GreenChannel:
        mode = "Green";
        penColor = QColor(0, 255, 0);
        break;
    case EHistogramDrawMode::BlueChannel:
        mode = "Blue";
        penColor = QColor(0, 0, 255);
        break;
    case EHistogramDrawMode::AlphaChannel:
        mode = "Alpha";
        penColor = QColor(120, 120, 120);
        break;
    }

    penSpikes = penColor;
    painter.setPen(Qt::black);
    painter.setBrush(Qt::white);
    rcGraph = QRect(QPoint(m_graphMargin, m_graphMargin), QPoint(abs(rc.width() - m_graphMargin), static_cast<int>(abs(rc.height() * m_graphHeightPercent))));
    painter.drawRect(rcGraph);
    painter.setPen(penSpikes);

    int i = 0;
    int graphWidth = rcGraph.width() != 0 ? abs(rcGraph.width()) : 1;
    int graphHeight = abs(rcGraph.height() - 1);
    int graphBottom = abs(rcGraph.top() + rcGraph.height());

    if (m_drawMode != EHistogramDrawMode::SplitRGB &&
        m_drawMode != EHistogramDrawMode::OverlappedRGB)
    {
        int crtX = 0;

        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            float scale = 0;

            i = static_cast<int>(((float)x / graphWidth) * (kNumColorLevels - 1));
            i = AZStd::clamp(i, 0, kNumColorLevels - 1);

            switch (m_drawMode)
            {
            case EHistogramDrawMode::Luminosity:
            {
                if (m_maxLumCount)
                {
                    scale = (float)m_lumCount[i] / m_maxLumCount;
                }
                break;
            }

            case EHistogramDrawMode::RedChannel:
            {
                if (m_maxCount[0])
                {
                    scale = (float)m_count[0][i] / m_maxCount[0];
                }
                break;
            }

            case EHistogramDrawMode::GreenChannel:
            {
                if (m_maxCount[1])
                {
                    scale = (float)m_count[1][i] / m_maxCount[1];
                }
                break;
            }

            case EHistogramDrawMode::BlueChannel:
            {
                if (m_maxCount[2])
                {
                    scale = (float)m_count[2][i] / m_maxCount[2];
                }
                break;
            }

            case EHistogramDrawMode::AlphaChannel:
            {
                if (m_maxCount[3])
                {
                    scale = (float)m_count[3][i] / m_maxCount[3];
                }
                break;
            }
            }

            crtX = static_cast<int>(rcGraph.left() + x + 1);
            painter.drawLine(crtX, graphBottom, crtX, static_cast<int>(graphBottom - scale * graphHeight));
        }
    }
    else
    if (m_drawMode == EHistogramDrawMode::OverlappedRGB)
    {
        int lastHeight[kNumChannels] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };
        int heightR, heightG, heightB, heightA;
        float scaleR, scaleG, scaleB, scaleA;
        UINT crtX, prevX = INT_MAX;

        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            i = static_cast<int>(((float)x / graphWidth) * (kNumColorLevels - 1));
            i = AZStd::clamp(i, 0, kNumColorLevels - 1);
            crtX = static_cast<UINT>(rcGraph.left() + x + 1);
            scaleR = scaleG = scaleB = scaleA = 0;

            if (m_maxCount[0])
            {
                scaleR = (float)m_count[0][i] / m_maxCount[0];
            }

            if (m_maxCount[1])
            {
                scaleG = (float)m_count[1][i] / m_maxCount[1];
            }

            if (m_maxCount[2])
            {
                scaleB = (float)m_count[2][i] / m_maxCount[2];
            }

            if (m_maxCount[3])
            {
                scaleA = (float)m_count[3][i] / m_maxCount[3];
            }

            heightR = static_cast<int>(graphBottom - scaleR * graphHeight);
            heightG = static_cast<int>(graphBottom - scaleG * graphHeight);
            heightB = static_cast<int>(graphBottom - scaleB * graphHeight);
            heightA = static_cast<int>(graphBottom - scaleA * graphHeight);

            if (lastHeight[0] == INT_MAX)
            {
                lastHeight[0] = heightR;
            }

            if (lastHeight[1] == INT_MAX)
            {
                lastHeight[1] = heightG;
            }

            if (lastHeight[2] == INT_MAX)
            {
                lastHeight[2] = heightB;
            }

            if (lastHeight[3] == INT_MAX)
            {
                lastHeight[3] = heightA;
            }

            if (prevX == INT_MAX)
            {
                prevX = crtX;
            }

            painter.setPen(redPen);
            painter.drawLine(prevX, lastHeight[0], crtX, heightR);

            painter.setPen(greenPen);
            painter.drawLine(prevX, lastHeight[1], crtX, heightG);

            painter.setPen(bluePen);
            painter.drawLine(prevX, lastHeight[2], crtX, heightB);

            painter.setPen(alphaPen);
            painter.drawLine(prevX, lastHeight[3], crtX, heightA);

            lastHeight[0] = heightR;
            lastHeight[1] = heightG;
            lastHeight[2] = heightB;
            lastHeight[3] = heightA;
            prevX = crtX;
        }
    }
    else
    if (m_drawMode == EHistogramDrawMode::SplitRGB)
    {
        const float aThird = 1.0f / 3.0f;
        const int aThirdOfNumColorLevels = kNumColorLevels / 3;
        const int aThirdOfWidth = rcGraph.width() / 3;
        QPen pPen;
        float scale = 0, pos = 0;

        // draw 3 blocks so we can see channel spaces
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kRedSectionColor);
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1 + aThirdOfWidth, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kGreenSectionColor);
        painter.fillRect(QRect(QPoint(rcGraph.left() + 1 + aThirdOfWidth * 2, rcGraph.top() + 1), QSize(aThirdOfWidth, rcGraph.height() - 2)), ImageHistogram::kBlueSectionColor);

        // 3 split RGB channel histograms
        for (size_t x = 0, xCount = abs(rcGraph.width()); x < xCount; ++x)
        {
            pos = (float)x / graphWidth;
            i = static_cast<int>((float)((int)(pos * kNumColorLevels) % aThirdOfNumColorLevels) / aThirdOfNumColorLevels * kNumColorLevels);
            i = AZStd::clamp(i, 0, kNumColorLevels - 1);
            scale = 0;

            // R
            if (pos < aThird)
            {
                if (m_maxCount[0])
                {
                    scale = (float)m_count[0][i] / m_maxCount[0];
                }
                pPen = redPen;
            }

            // G
            if (pos > aThird && pos < aThird * 2)
            {
                if (m_maxCount[1])
                {
                    scale = (float) m_count[1][i] / m_maxCount[1];
                }
                pPen = greenPen;
            }

            // B
            if (pos > aThird * 2)
            {
                if (m_maxCount[2])
                {
                    scale = (float) m_count[2][i] / m_maxCount[2];
                }
                pPen = bluePen;
            }

            painter.setPen(pPen);
            painter.drawLine(rcGraph.left() + static_cast<int>(x) + 1, graphBottom, rcGraph.left() + static_cast<int>(x) + 1, static_cast<int>(graphBottom - scale * graphHeight));
        }

        // then draw 3 lines so we separate the channels
        QPen wallPen(ImageHistogram::kSplitSeparatorColor, 1, Qt::DotLine);
        painter.setPen(wallPen);
        painter.drawLine(rcGraph.left() + aThirdOfWidth, rcGraph.bottom(), rcGraph.left() + aThirdOfWidth, rcGraph.top());
        painter.drawLine(rcGraph.left() + aThirdOfWidth * 2, rcGraph.bottom(), rcGraph.left() + aThirdOfWidth * 2, rcGraph.top());
    }

    QRect rcText;
    rcText = QRect(QPoint(m_graphMargin, rcGraph.height() + m_graphMargin * 2),
                   QPoint(rc.width(), rc.height() - m_graphMargin));

    float mean = 0, stdDev = 0, median = 0;

    switch (m_drawMode)
    {
    case EHistogramDrawMode::Luminosity:
    case EHistogramDrawMode::SplitRGB:
    case EHistogramDrawMode::OverlappedRGB:
        mean = m_meanAvg;
        stdDev = m_stdDevAvg;
        median = m_medianAvg;
        break;
    case EHistogramDrawMode::RedChannel:
        mean = m_mean[0];
        stdDev = m_stdDev[0];
        median = m_median[0];
        break;
    case EHistogramDrawMode::GreenChannel:
        mean = m_mean[1];
        stdDev = m_stdDev[1];
        median = m_median[1];
        break;
    case EHistogramDrawMode::BlueChannel:
        mean = m_mean[2];
        stdDev = m_stdDev[2];
        median = m_median[2];
        break;
    case EHistogramDrawMode::AlphaChannel:
        mean = m_mean[3];
        stdDev = m_stdDev[3];
        median = m_median[3];
        break;
    }

    str = tr("Mean: %1 StdDev: %2 Median: %3").arg(mean, 0, 'f', 2).arg(stdDev, 0, 'f', 2).arg(median, 0, 'f', 2);
    fnt = QFont(ImageHistogram::kTextFontFace, ImageHistogram::kTextFontSize / 10);
    painter.setFont(fnt);
    painter.setPen(ImageHistogram::kTextColor);
    painter.drawText(rcText, Qt::AlignCenter | Qt::TextSingleLine, painter.fontMetrics().elidedText(str, Qt::ElideRight, rcText.width(), Qt::TextSingleLine));
}

CImageHistogramDisplay* CImageHistogramCtrl::histogramDisplay() const
{
    return m_display;
}

#include <Controls/moc_ImageHistogramCtrl.cpp>
