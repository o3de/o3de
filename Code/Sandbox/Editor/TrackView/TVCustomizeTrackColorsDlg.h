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

// Description : A dialog for customizing track colors


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TVCUSTOMIZETRACKCOLORSDLG_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TVCUSTOMIZETRACKCOLORSDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class TVCustomizeTrackColorsDialog;
}

class QLabel;
class ColorButton;

class CTVCustomizeTrackColorsDlg
    : public QDialog
{
    Q_OBJECT
    friend class CTrackViewDialog;
public:
    CTVCustomizeTrackColorsDlg(QWidget* pParent = nullptr);
    virtual ~CTVCustomizeTrackColorsDlg();

    static QColor GetTrackColor(CAnimParamType paramType)
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

    inline void GetQColorFromXmlNode(QColor& colorOut, const XmlNodeRef& xmlNode) const
    {
        QRgb rgb = -1;
        xmlNode->getAttr("color", rgb);
        colorOut.setRgb(rgb);
    };

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

    QScopedPointer<Ui::TVCustomizeTrackColorsDialog> m_ui;

    static std::map<CAnimParamType, QColor> s_trackColors;
    static QColor s_colorForDisabled;
    static QColor s_colorForMuted;
    static QColor s_colorForOthers;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TVCUSTOMIZETRACKCOLORSDLG_H
