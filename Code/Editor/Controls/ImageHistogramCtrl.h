/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)

#include "Util/Image.h"
#include "Util/ImageHistogram.h"

#include <QWidget>
#endif

class QComboBox;
class QLabel;

enum class EHistogramDrawMode
{
    Luminosity,
    OverlappedRGB,
    SplitRGB,
    RedChannel,
    GreenChannel,
    BlueChannel,
    AlphaChannel
};
Q_ENUMS(EHistogramDrawMode)
Q_DECLARE_METATYPE(EHistogramDrawMode)


class SANDBOX_API CImageHistogramDisplay
    : public QWidget
    , public CImageHistogram
{
    Q_OBJECT
public:
    CImageHistogramDisplay(QWidget* parent = nullptr);
    virtual ~CImageHistogramDisplay();

    void paintEvent(QPaintEvent* event) override;

    EHistogramDrawMode  m_drawMode;
    int                 m_graphMargin;
    float               m_graphHeightPercent;
    QColor              m_backColor;
};

class SANDBOX_API CImageHistogramCtrl
    : public QWidget
{
    Q_OBJECT

public:
    CImageHistogramCtrl(QWidget* parent = nullptr);
    virtual ~CImageHistogramCtrl();

    EHistogramDrawMode drawMode() const;
    void setDrawMode(EHistogramDrawMode drawMode);

    void ComputeHistogram(CImageEx& img, CImageHistogram::EImageFormat format);

    CImageHistogramDisplay* histogramDisplay() const;

private:
    CImageHistogramDisplay* m_display;
    QComboBox*              m_drawMode;
    QLabel*                 m_infoText;
};



#endif // CRYINCLUDE_EDITOR_CONTROLS_IMAGEHISTOGRAMCTRL_H
