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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
