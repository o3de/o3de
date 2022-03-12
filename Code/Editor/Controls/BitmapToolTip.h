/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Tooltip that displays bitmap.


#ifndef CRYINCLUDE_EDITOR_CONTROLS_BITMAPTOOLTIP_H
#define CRYINCLUDE_EDITOR_CONTROLS_BITMAPTOOLTIP_H
#pragma once


#if !defined(Q_MOC_RUN)
#include "Controls/ImageHistogramCtrl.h"

#include <QLabel>
#include <QTimer>
#endif

//////////////////////////////////////////////////////////////////////////
class CBitmapToolTip
    : public QWidget
{
    Q_OBJECT
    // Construction
public:

    enum EShowMode
    {
        ESHOW_RGB = 0,
        ESHOW_ALPHA,
        ESHOW_RGBA,
        ESHOW_RGB_ALPHA,
        ESHOW_RGBE
    };

    CBitmapToolTip(QWidget* parent = nullptr);
    virtual ~CBitmapToolTip();

    bool Create(const RECT& rect);

    // Attributes
public:

    // Operations
public:
    void RefreshViewmode();

    bool LoadImage(const QString& imageFilename);
    void SetTool(QWidget* pWnd, const QRect& rect);

    // Generated message map functions
protected:
    void OnTimer();

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void GetShowMode(EShowMode& showMode, bool& showInOriginalSize) const;
    const char* GetShowModeDescription(EShowMode showMode, bool showInOriginalSize) const;

    QLabel* m_staticBitmap;
    QLabel* m_staticText;
    QString m_filename;
    bool        m_bShowHistogram;
    EShowMode m_eShowMode;
    bool    m_bShowFullsize;
    bool    m_bHasAlpha;
    bool    m_bIsLimitedHDR;
    CImageHistogramCtrl* m_rgbaHistogram;
    CImageHistogramCtrl* m_alphaChannelHistogram;
    int m_nTimer;
    QWidget* m_hToolWnd;
    QRect m_toolRect;
    QTimer m_timer;
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_BITMAPTOOLTIP_H
