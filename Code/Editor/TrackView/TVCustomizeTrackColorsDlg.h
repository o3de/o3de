/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for customizing track colors


#pragma once

#if !defined(Q_MOC_RUN)
#include <QColor>
#include <QDialog>
#endif
#include <AzCore/std/containers/map.h>

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
        return s_colorForOthers;
    }

    static QColor GetColorForDisabledTracks() { return s_colorForDisabled; }
    static QColor GetColorForMutedTracks() { return s_colorForMuted; }

private:

    inline void GetQColorFromXmlNode(QColor& colorOut, const XmlNodeRef& xmlNode) const
    {
        QRgb rgb = std::numeric_limits<unsigned int>::max();
        xmlNode->getAttr("color", rgb);
        colorOut.setRgb(rgb);
    }

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

    static AZStd::map<CAnimParamType, QColor> s_trackColors;
    static QColor s_colorForDisabled;
    static QColor s_colorForMuted;
    static QColor s_colorForOthers;
};
