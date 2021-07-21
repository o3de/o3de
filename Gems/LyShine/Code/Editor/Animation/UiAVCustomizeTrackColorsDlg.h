/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QScopedPointer>

#include <LyShine/Animation/IUiAnimation.h>
#endif

namespace Ui
{
    class UiAVCustomizeTrackColorsDialog;
}

class QLabel;

class ColorButton;

class CUiAVCustomizeTrackColorsDlg
    : public QDialog
{
    Q_OBJECT
    friend class CUiAnimViewDialog;
public:
    CUiAVCustomizeTrackColorsDlg(QWidget* pParent = nullptr);
    virtual ~CUiAVCustomizeTrackColorsDlg();

    static QColor GetTrackColor(CUiAnimParamType paramType)
    {
        auto itr = s_trackColors.find(paramType);
        if (itr != end(s_trackColors))
        {
            return itr->second;
        }
        else
        {
            return s_colorForOthers;
        }
    }
    static QColor GetColorForDisabledTracks()
    { return s_colorForDisabled; }
    static QColor GetColorForMutedTracks()
    { return s_colorForMuted; }

private:
    virtual void OnInitDialog();
    void OnApply();
    void OnResetAll();
    void OnExport();
    void OnImport();
    void accept() override;

    void Export(const QString& fullPath) const;
    bool Import(const QString& fullPath);

    static void SaveColors(const char* sectionName);
    static void LoadColors(const char* sectionName);

    QVector<QLabel*> m_aLabels;
    QVector<ColorButton*> m_colorButtons;

    QScopedPointer<Ui::UiAVCustomizeTrackColorsDialog> m_ui;

    static std::map<CUiAnimParamType, QColor> s_trackColors;
    static QColor s_colorForDisabled;
    static QColor s_colorForMuted;
    static QColor s_colorForOthers;
};
