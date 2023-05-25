/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



//----- UI_ANIMATION_REVISIT - this is required to compile since something we include still uses MFC

// prevent inclusion of conflicting definitions of INT8_MIN etc
#define _INTSAFE_H_INCLUDED_

// ----- End UI_ANIMATION_REVISIT

#include "EditorDefs.h"


#include <algorithm>
#include "UiAVCustomizeTrackColorsDlg.h"
#include "UiAnimViewDialog.h"

#include <Editor/Animation/ui_UiAVCustomizeTrackColorsDialog.h>

#include <QLabel>
#include <QMessageBox>

#include <QtUtilWin.h>

#include "UiEditorAnimationBus.h"

#define TRACKCOLOR_ENTRY_PREFIX _T("TrackColor")
#define TRACKCOLOR_FOR_OTHERS_ENTRY _T("TrackColorForOthers")
#define TRACKCOLOR_FOR_DISABLED_ENTRY _T("TrackColorForDisabled")
#define TRACKCOLOR_FOR_MUTED_ENTRY _T("TrackColorForMuted")

#include "QtUI/ColorButton.h"

struct UiAnimTrackEntry
{
    CUiAnimParamType paramType;
    QString name;
    QColor defaultColor;
};

namespace
{
    const UiAnimTrackEntry g_trackEntries[] = {
        // Color for tracks
        { eUiAnimParamType_AzComponentField, "AzComponentField", QColor(220, 220, 220) },
        { eUiAnimParamType_Event, "Event", QColor(220, 220, 220) },
        { eUiAnimParamType_TrackEvent, "TrackEvent", QColor(220, 220, 220) },
        { eUiAnimParamType_Float, "Float", QColor(220, 220, 220) },
        { eUiAnimParamType_ByString, "ByString", QColor(220, 220, 220) },

        { eUiAnimParamType_User, "", QColor(0, 0, 0) }, // An empty string means a separator row.

        // Misc colors for special states of a track
        { eUiAnimParamType_User, "Others", QColor(220, 220, 220) },
        { eUiAnimParamType_User, "Disabled/Inactive", QColor(255, 224, 224) },
        { eUiAnimParamType_User, "Muted", QColor(255, 224, 224) },
    };

    const int kMaxRows = 20;
    const int kColumnWidth = 300;
    const int kRowHeight = 24;

    const int kOthersEntryIndex = arraysize(g_trackEntries) - 3;
    const int kDisabledEntryIndex = arraysize(g_trackEntries) - 2;
    const int kMutedEntryIndex = arraysize(g_trackEntries) - 1;
}

std::map<CUiAnimParamType, QColor> CUiAVCustomizeTrackColorsDlg::s_trackColors;
QColor CUiAVCustomizeTrackColorsDlg::s_colorForDisabled;
QColor CUiAVCustomizeTrackColorsDlg::s_colorForMuted;
QColor CUiAVCustomizeTrackColorsDlg::s_colorForOthers;

CUiAVCustomizeTrackColorsDlg::CUiAVCustomizeTrackColorsDlg(QWidget* pParent)
    : QDialog(pParent)
    , m_aLabels(arraysize(g_trackEntries))
    , m_colorButtons(arraysize(g_trackEntries))
    , m_ui(new Ui::UiAVCustomizeTrackColorsDialog)
{
    OnInitDialog();
}

CUiAVCustomizeTrackColorsDlg::~CUiAVCustomizeTrackColorsDlg()
{
}

void CUiAVCustomizeTrackColorsDlg::OnInitDialog()
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &CUiAVCustomizeTrackColorsDlg::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &CUiAVCustomizeTrackColorsDlg::reject);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &CUiAVCustomizeTrackColorsDlg::OnApply);
    connect(m_ui->buttonResetAll, &QPushButton::clicked, this, &CUiAVCustomizeTrackColorsDlg::OnResetAll);
    connect(m_ui->buttonExport, &QPushButton::clicked, this, &CUiAVCustomizeTrackColorsDlg::OnExport);
    connect(m_ui->buttonImport, &QPushButton::clicked, this, &CUiAVCustomizeTrackColorsDlg::OnImport);


    QRect labelRect(QPoint(30, 30), QPoint(150, 50));
    QRect buttonRect(QPoint(180, 30), QPoint(280, 50));
    // Create a label and a color button for each track.
    int col = 0, i = 0;
    std::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const UiAnimTrackEntry& entry)
        {
            const QString labelText = entry.name;

            if (!labelText.isEmpty())
            {
                m_aLabels[i] = new QLabel(m_ui->frame);
                m_aLabels[i]->setGeometry(labelRect);
                m_aLabels[i]->setText(labelText);

                m_colorButtons[i] = new ColorButton(m_ui->frame);
                m_colorButtons[i]->setGeometry(buttonRect);

                if (entry.paramType.GetType() == eUiAnimParamType_User)
                {
                    assert(kOthersEntryIndex <= i);
                    if (i == kOthersEntryIndex)
                    {
                        m_colorButtons[i]->SetColor(s_colorForOthers);
                    }
                    else if (i == kDisabledEntryIndex)
                    {
                        m_colorButtons[i]->SetColor(s_colorForDisabled);
                    }
                    else if (i == kMutedEntryIndex)
                    {
                        m_colorButtons[i]->SetColor(s_colorForMuted);
                    }
                }
                else
                {
                    m_colorButtons[i]->SetColor(s_trackColors[entry.paramType]);
                }
            }

            if (i % kMaxRows == kMaxRows - 1)
            {
                ++col;
                labelRect.moveTopLeft(QPoint(30 + kColumnWidth * col, 30));
                buttonRect.moveTopLeft(QPoint(180 + kColumnWidth * col, 30));
            }
            else
            {
                labelRect.translate(0, kRowHeight);
                buttonRect.translate(0, kRowHeight);
            }
            ++i;
        });

    // Resize this dialog to fit the contents.
    const QSize size(60 + kColumnWidth * (col + 1), 100 + kMaxRows * kRowHeight);
    m_ui->frame->setFixedSize(size);
    setFixedSize(sizeHint());
}

void CUiAVCustomizeTrackColorsDlg::accept()
{
    OnApply();
    QDialog::accept();
}

void CUiAVCustomizeTrackColorsDlg::OnApply()
{
    int i = 0;
    std::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const UiAnimTrackEntry& entry)
        {
            if (entry.paramType.GetType() != eUiAnimParamType_User)
            {
                s_trackColors[entry.paramType] = m_colorButtons[i]->Color();
            }
            ++i;
        });

    s_colorForOthers = m_colorButtons[kOthersEntryIndex]->Color();
    s_colorForDisabled = m_colorButtons[kDisabledEntryIndex]->Color();
    s_colorForMuted = m_colorButtons[kMutedEntryIndex]->Color();

    CUiAnimViewDialog::GetCurrentInstance()->InvalidateDopeSheet();
}

void CUiAVCustomizeTrackColorsDlg::OnResetAll()
{
    int i = 0;
    std::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const UiAnimTrackEntry& entry)
        {
            const QString labelText = entry.name;
            if (!labelText.isEmpty())
            {
                m_colorButtons[i]->SetColor(entry.defaultColor);
            }
            ++i;
        });
}

void CUiAVCustomizeTrackColorsDlg::SaveColors([[maybe_unused]] const char* sectionName)
{
    // We no longer support custom colors but haven't completely removed the functionality
    // in case we want to support it.
}

void CUiAVCustomizeTrackColorsDlg::LoadColors([[maybe_unused]] const char* sectionName)
{
    // We no longer support custom colors but haven't completely removed the functionality
    // in case we want to support it. For now we just initialize the colors in this function
    std::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const UiAnimTrackEntry& entry)
        {
            if (entry.paramType.GetType() != eUiAnimParamType_User)
            {
                s_trackColors[entry.paramType] = entry.defaultColor;
            }
        });

    s_colorForOthers = g_trackEntries[kOthersEntryIndex].defaultColor.rgb();
    s_colorForDisabled = g_trackEntries[kDisabledEntryIndex].defaultColor.rgb();
    s_colorForMuted = g_trackEntries[kMutedEntryIndex].defaultColor.rgb();
}

void CUiAVCustomizeTrackColorsDlg::OnExport()
{
#if UI_ANIMATION_REMOVED
    QString savePath;
    if (CFileUtil::SelectSaveFile("Custom Track Colors Files (*.ctc)", "ctc",
            Path::GetUserSandboxFolder(), savePath))
    {
        Export(savePath);
    }
#endif
}

void CUiAVCustomizeTrackColorsDlg::OnImport()
{
#if UI_ANIMATION_REMOVED
    QString loadPath;
    if (CFileUtil::SelectFile("Custom Track Colors Files (*.ctc)",
            Path::GetUserSandboxFolder(), loadPath))
    {
        if (Import(loadPath) == false)
        {
            QMessageBox::critical(this, tr("Cannot import"), tr("The file format is invalid!"));
        }
    }
#endif
}

void CUiAVCustomizeTrackColorsDlg::Export(const QString& fullPath) const
{
    XmlNodeRef customTrackColorsNode = XmlHelpers::CreateXmlNode("customtrackcolors");

    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    int i = 0;
    std::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const UiAnimTrackEntry& entry)
        {
            if (entry.paramType.GetType() != eUiAnimParamType_User)
            {
                XmlNodeRef entryNode = customTrackColorsNode->newChild("entry");

                // Serialization is const safe
                CUiAnimParamType& paramType = const_cast<CUiAnimParamType&>(entry.paramType);
                paramType.Serialize(animationSystem, entryNode, false);
                entryNode->setAttr("color", m_colorButtons[i]->Color().rgb());
            }
            ++i;
        });

    XmlNodeRef othersNode = customTrackColorsNode->newChild("others");
    othersNode->setAttr("color", m_colorButtons[kOthersEntryIndex]->Color().rgb());
    XmlNodeRef disabledNode = customTrackColorsNode->newChild("disabled");
    disabledNode->setAttr("color", m_colorButtons[kDisabledEntryIndex]->Color().rgb());
    XmlNodeRef mutedNode = customTrackColorsNode->newChild("muted");
    mutedNode->setAttr("color", m_colorButtons[kMutedEntryIndex]->Color().rgb());

    XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), customTrackColorsNode, fullPath.toStdString().c_str());
}

bool CUiAVCustomizeTrackColorsDlg::Import(const QString& fullPath)
{
    IUiAnimationSystem* animationSystem = nullptr;
    UiEditorAnimationBus::BroadcastResult(animationSystem, &UiEditorAnimationBus::Events::GetAnimationSystem);

    XmlNodeRef customTrackColorsNode = XmlHelpers::LoadXmlFromFile(fullPath.toStdString().c_str());
    if (customTrackColorsNode == NULL)
    {
        return false;
    }

    for (int i = 0; i < customTrackColorsNode->getChildCount(); ++i)
    {
        XmlNodeRef childNode = customTrackColorsNode->getChild(i);
        if (QString(childNode->getTag()) != "entry")
        {
            continue;
        }

        CUiAnimParamType paramType;
        if (childNode->haveAttr("paramtype"))
        {
            int paramId;
            childNode->getAttr("paramtype", paramId);
            paramType = paramId;
        }
        else
        {
            paramType.Serialize(animationSystem, childNode, true);
        }

        // Get the entry index for this param type.
        const UiAnimTrackEntry* pEntry = std::find_if(g_trackEntries, g_trackEntries + arraysize(g_trackEntries),
                [=](const UiAnimTrackEntry& entry)
                {
                    return entry.paramType == paramType;
                });
        int entryIndex = static_cast<int>(pEntry - g_trackEntries);
        if (entryIndex >= arraysize(g_trackEntries)) // If not found, skip this.
        {
            continue;
        }
        COLORREF color = std::numeric_limits<COLORREF>::max();
        childNode->getAttr("color", color);
        m_colorButtons[entryIndex]->SetColor(color);
    }

    XmlNodeRef othersNode = customTrackColorsNode->findChild("others");
    if (othersNode)
    {
        COLORREF color = std::numeric_limits<COLORREF>::max();
        othersNode->getAttr("color", color);
        m_colorButtons[kOthersEntryIndex]->SetColor(color);
    }

    XmlNodeRef disabledNode = customTrackColorsNode->findChild("disabled");
    if (disabledNode)
    {
        COLORREF color = std::numeric_limits<COLORREF>::max();
        disabledNode->getAttr("color", color);
        m_colorButtons[kDisabledEntryIndex]->SetColor(color);
    }

    XmlNodeRef mutedNode = customTrackColorsNode->findChild("muted");
    if (mutedNode)
    {
        COLORREF color = std::numeric_limits<COLORREF>::max();
        mutedNode->getAttr("color", color);
        m_colorButtons[kMutedEntryIndex]->SetColor(color);
    }

    return true;
}

#include <Animation/moc_UiAVCustomizeTrackColorsDlg.cpp>
