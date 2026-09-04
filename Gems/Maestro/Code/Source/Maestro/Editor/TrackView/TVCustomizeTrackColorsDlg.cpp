/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A dialog for customizing track colors


#include "EditorDefs.h"

#include "TVCustomizeTrackColorsDlg.h"

// Qt
#include <QLabel>
#include <QMessageBox>
#include <QSettings>

// AzCore
#include <AzCore/std/algorithm.h>

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "TrackViewDialog.h"
#include "QtUI/ColorButton.h"

#include <TrackView/ui_TVCustomizeTrackColorsDialog.h>

#define TRACKCOLOR_ENTRY_PREFIX ("TrackColor")
#define TRACKCOLOR_FOR_OTHERS_ENTRY ("TrackColorForOthers")
#define TRACKCOLOR_FOR_DISABLED_ENTRY ("TrackColorForDisabled")
#define TRACKCOLOR_FOR_MUTED_ENTRY ("TrackColorForMuted")

struct STrackEntry
{
    CAnimParamType paramType;
    const char* name;
    QColor defaultColor;
};

namespace
{
    const STrackEntry g_trackEntries[] = {
        // Color for tracks
        { AnimParamType::Position, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Pos"), QColor(90, 150, 90) },
        { AnimParamType::Rotation, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Rot"), QColor(90, 150, 90) },
        { AnimParamType::Scale, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Scale"), QColor(90, 150, 90) },
        { AnimParamType::Event, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Event"), QColor(220, 220, 220) },
        { AnimParamType::Visibility, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Visibility"), QColor(220, 220, 220) },
        { AnimParamType::Camera, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Camera"), QColor(220, 220, 220) },
        { AnimParamType::Sound, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Sound"), QColor(220, 220, 220) },
        { AnimParamType::Animation, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Animation"), QColor(220, 220, 220) },
        { AnimParamType::Sequence, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Sequence"), QColor(220, 220, 220) },
        { AnimParamType::Console, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Console"), QColor(220, 220, 220) },
        { AnimParamType::LookAt, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LookAt"), QColor(220, 220, 220) },
        { AnimParamType::TrackEvent, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "TrackEvent"), QColor(220, 220, 220) },
        { AnimParamType::ShakeMultiplier, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ShakeMult"), QColor(90, 150, 90) },
        { AnimParamType::TransformNoise, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Noise"), QColor(90, 150, 90) },
        { AnimParamType::TimeWarp, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Timewarp"), QColor(220, 220, 220) },
        { AnimParamType::FixedTimeStep, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "FixedTimeStep"), QColor(220, 220, 220) },
        { AnimParamType::DepthOfField, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "DepthOfField"), QColor(90, 150, 90) },
        { AnimParamType::CommentText, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "CommentText"), QColor(220, 220, 220) },
        { AnimParamType::ScreenFader, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ScreenFader"), QColor(220, 220, 220) },
        { AnimParamType::LightDiffuse, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightDiffuseColor"), QColor(90, 150, 90) },
        { AnimParamType::LightRadius, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightRadius"), QColor(220, 220, 220) },
        { AnimParamType::LightDiffuseMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightDiffuseMult"), QColor(220, 220, 220) },
        { AnimParamType::LightHDRDynamic, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightHDRDynamic"), QColor(220, 220, 220) },
        { AnimParamType::LightSpecularMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightSpecularMult"), QColor(220, 220, 220) },
        { AnimParamType::LightSpecPercentage, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "LightSpecularPercent"), QColor(220, 220, 220) },
        { AnimParamType::FocusDistance, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "FocusDistance"), QColor(220, 220, 220) },
        { AnimParamType::FocusRange, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "FocusRange"), QColor(220, 220, 220) },
        { AnimParamType::BlurAmount, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "BlurAmount"), QColor(220, 220, 220) },
        { AnimParamType::PositionX, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "PosX"), QColor(220, 220, 220) },
        { AnimParamType::PositionY, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "PosY"), QColor(220, 220, 220) },
        { AnimParamType::PositionZ, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "PosZ"), QColor(220, 220, 220) },
        { AnimParamType::RotationX, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "RotX"), QColor(220, 220, 220) },
        { AnimParamType::RotationY, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "RotY"), QColor(220, 220, 220) },
        { AnimParamType::RotationZ, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "RotZ"), QColor(220, 220, 220) },
        { AnimParamType::ScaleX, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ScaleX"), QColor(220, 220, 220) },
        { AnimParamType::ScaleY, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ScaleY"), QColor(220, 220, 220) },
        { AnimParamType::ScaleZ, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ScaleZ"), QColor(220, 220, 220) },
        { AnimParamType::ShakeAmpAMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ShakeMultAmpA"), QColor(220, 220, 220) },
        { AnimParamType::ShakeAmpBMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ShakeMultAmpB"), QColor(220, 220, 220) },
        { AnimParamType::ShakeFreqAMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ShakeMultFreqA"), QColor(220, 220, 220) },
        { AnimParamType::ShakeFreqBMult, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ShakeMultFreqB"), QColor(220, 220, 220) },
        { AnimParamType::ColorR, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ColorR"), QColor(220, 220, 220) },
        { AnimParamType::ColorG, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ColorG"), QColor(220, 220, 220) },
        { AnimParamType::ColorB, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "ColorB"), QColor(220, 220, 220) },
        { AnimParamType::MaterialOpacity, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "MaterialOpacity"), QColor(220, 220, 220) },
        { AnimParamType::MaterialSmoothness, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "MaterialGlossiness"), QColor(220, 220, 220) },
        { AnimParamType::MaterialEmissive, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "MaterialEmission"), QColor(220, 220, 220) },
        { AnimParamType::MaterialEmissiveIntensity, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "MaterialEmissionIntensity"), QColor(220, 220, 220) },
        { AnimParamType::NearZ, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "NearZ"), QColor(220, 220, 220) },

        { AnimParamType::User, "", QColor(0, 0, 0) }, // An empty string means a separator row.

        // Misc colors for special states of a track
        { AnimParamType::User, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Others"), QColor(220, 220, 220) },
        { AnimParamType::User, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Disabled/Inactive"), QColor(255, 224, 224) },
        { AnimParamType::User, QT_TRANSLATE_NOOP("CTVCustomizeTrackColorsDlg", "Muted"), QColor(255, 224, 224) },
    };

    const int kMaxRows = 20;
    const int kColumnWidth = 300;
    const int kRowHeight = 24;

    const int kOthersEntryIndex = arraysize(g_trackEntries) - 3;
    const int kDisabledEntryIndex = arraysize(g_trackEntries) - 2;
    const int kMutedEntryIndex = arraysize(g_trackEntries) - 1;
}

AZStd::map<CAnimParamType, QColor> CTVCustomizeTrackColorsDlg::s_trackColors;
QColor CTVCustomizeTrackColorsDlg::s_colorForDisabled;
QColor CTVCustomizeTrackColorsDlg::s_colorForMuted;
QColor CTVCustomizeTrackColorsDlg::s_colorForOthers;

CTVCustomizeTrackColorsDlg::CTVCustomizeTrackColorsDlg(QWidget* pParent)
    : QDialog(pParent)
    , m_aLabels(arraysize(g_trackEntries))
    , m_colorButtons(arraysize(g_trackEntries))
    , m_ui(new Ui::TVCustomizeTrackColorsDialog)
{
    OnInitDialog();
}

CTVCustomizeTrackColorsDlg::~CTVCustomizeTrackColorsDlg()
{
}

void CTVCustomizeTrackColorsDlg::OnInitDialog()
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &CTVCustomizeTrackColorsDlg::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &CTVCustomizeTrackColorsDlg::reject);
    connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &CTVCustomizeTrackColorsDlg::OnApply);
    connect(m_ui->buttonResetAll, &QPushButton::clicked, this, &CTVCustomizeTrackColorsDlg::OnResetAll);
    connect(m_ui->buttonExport, &QPushButton::clicked, this, &CTVCustomizeTrackColorsDlg::OnExport);
    connect(m_ui->buttonImport, &QPushButton::clicked, this, &CTVCustomizeTrackColorsDlg::OnImport);


    QRect labelRect(QPoint(30, 30), QPoint(150, 50));
    QRect buttonRect(QPoint(180, 30), QPoint(280, 50));
    // Create a label and a color button for each track.
    int col = 0, i = 0;
    AZStd::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const STrackEntry& entry)
    {
        const QString labelText = entry.name;

        if(!labelText.isEmpty())
        {
            m_aLabels[i] = new QLabel(m_ui->frame);
            m_aLabels[i]->setGeometry(labelRect);
            m_aLabels[i]->setText(tr(entry.name));

            m_colorButtons[i] = new ColorButton(m_ui->frame);
            m_colorButtons[i]->setGeometry(buttonRect);

            if(entry.paramType.GetType() == AnimParamType::User)
            {
                AZ_Assert(kOthersEntryIndex <= i, "Button color is %i is out of range", i);
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

        if(i % kMaxRows == kMaxRows - 1)
        {
            ++col;
            labelRect.moveTopLeft(QPoint(30+kColumnWidth*col, 30));
            buttonRect.moveTopLeft(QPoint(180+kColumnWidth*col, 30));
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

void CTVCustomizeTrackColorsDlg::accept()
{
    OnApply();
    QDialog::accept();
}

void CTVCustomizeTrackColorsDlg::OnApply()
{
    int i = 0;
    AZStd::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const STrackEntry& entry)
    {
        if(entry.paramType.GetType() != AnimParamType::User)
        {
                s_trackColors[entry.paramType] = m_colorButtons[i]->Color();
        }
        ++i;
    });

    s_colorForOthers = m_colorButtons[kOthersEntryIndex]->Color();
    s_colorForDisabled = m_colorButtons[kDisabledEntryIndex]->Color();
    s_colorForMuted = m_colorButtons[kMutedEntryIndex]->Color();

    CTrackViewDialog::GetCurrentInstance()->InvalidateDopeSheet();
}

void CTVCustomizeTrackColorsDlg::OnResetAll()
{
    int i = 0;
    AZStd::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const STrackEntry& entry)
    {
        const QString labelText = entry.name;
        if(!labelText.isEmpty())
        {
            m_colorButtons[i]->SetColor(entry.defaultColor);
        }
        ++i;
    });
}

void CTVCustomizeTrackColorsDlg::SaveColors(const char* sectionName)
{
    QSettings settings;
    for (auto g : QString(sectionName).split('\\'))
    {
        settings.beginGroup(g);
    }
    AZStd::for_each(begin(s_trackColors), end(s_trackColors),
    [&](const AZStd::pair<CAnimParamType, QColor>& pair)
    {
        const QString trackColorEntry = QString::fromLatin1("%1%2").arg(TRACKCOLOR_ENTRY_PREFIX).arg(static_cast<int>(pair.first.GetType()));
        settings.setValue(trackColorEntry, pair.second.rgb());
    });

    settings.setValue(TRACKCOLOR_FOR_OTHERS_ENTRY, s_colorForOthers.rgb());
    settings.setValue(TRACKCOLOR_FOR_DISABLED_ENTRY, s_colorForDisabled.rgb());
    settings.setValue(TRACKCOLOR_FOR_MUTED_ENTRY, s_colorForMuted.rgb());
}

void CTVCustomizeTrackColorsDlg::LoadColors(const char* sectionName)
{
    QSettings settings;
    for (auto g : QString(sectionName).split('\\'))
    {
        settings.beginGroup(g);
    }
    AZStd::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const STrackEntry& entry)
    {
        if (entry.paramType.GetType() != AnimParamType::User)
        {
            s_trackColors[entry.paramType] = QColor::fromRgb(settings.value(QStringLiteral("%2%3").arg(TRACKCOLOR_ENTRY_PREFIX).arg(static_cast<int>(entry.paramType.GetType())), entry.defaultColor.rgb()).toInt());
        }
    });

    s_colorForOthers = QColor::fromRgb(settings.value(TRACKCOLOR_FOR_OTHERS_ENTRY, g_trackEntries[kOthersEntryIndex].defaultColor.rgb()).toInt());
    s_colorForDisabled = QColor::fromRgb(settings.value(TRACKCOLOR_FOR_DISABLED_ENTRY, g_trackEntries[kDisabledEntryIndex].defaultColor.rgb()).toInt());
    s_colorForMuted = QColor::fromRgb(settings.value(TRACKCOLOR_FOR_MUTED_ENTRY, g_trackEntries[kMutedEntryIndex].defaultColor.rgb()).toInt());
}

void CTVCustomizeTrackColorsDlg::OnExport()
{
    QString savePath;
    if (CFileUtil::SelectSaveFile(tr("Custom Track Colors Files (*.ctc)"), "ctc",
            Path::GetUserSandboxFolder(), savePath))
    {
        Export(savePath);
    }
}

void CTVCustomizeTrackColorsDlg::OnImport()
{
    QString loadPath;
    if (CFileUtil::SelectFile(tr("Custom Track Colors Files (*.ctc)"),
            Path::GetUserSandboxFolder(), loadPath))
    {
        if (Import(loadPath))
        {
            // since the user is explicitly pressing 'Import', we assume he or she wants to apply this import
            // to see the result immediately, based on a customer feedback sample of one
            OnApply();
        }
        else
        {
            QMessageBox::critical(this, tr("Cannot import"), tr("The file format is invalid!"));
        }
    }
}

void CTVCustomizeTrackColorsDlg::Export(const QString& fullPath) const
{
    XmlNodeRef customTrackColorsNode = XmlHelpers::CreateXmlNode("customtrackcolors");

    int i = 0;
    AZStd::for_each(g_trackEntries, g_trackEntries + arraysize(g_trackEntries), [&](const STrackEntry& entry)
    {
        if(entry.paramType.GetType() != AnimParamType::User)
        {
            XmlNodeRef entryNode = customTrackColorsNode->newChild("entry");

            // Serialization is const safe
            CAnimParamType &paramType = const_cast<CAnimParamType&>( entry.paramType );
            paramType.Serialize( entryNode, false );
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

bool CTVCustomizeTrackColorsDlg::Import(const QString& fullPath)
{
    XmlNodeRef customTrackColorsNode = XmlHelpers::LoadXmlFromFile(fullPath.toStdString().c_str());
    if (customTrackColorsNode == nullptr)
    {
        return false;
    }

    QColor color;
    for (int i = 0; i < customTrackColorsNode->getChildCount(); ++i)
    {
        XmlNodeRef childNode = customTrackColorsNode->getChild(i);
        if (QString(childNode->getTag()) != "entry")
        {
            continue;
        }

        CAnimParamType paramType;
        paramType.Serialize(childNode, true);

        // Get the entry index for this param type.
        const STrackEntry* pEntry = std::find_if(g_trackEntries, g_trackEntries + arraysize(g_trackEntries),
                [=](const STrackEntry& entry)
        {
            return entry.paramType == paramType;
        });
        int entryIndex = static_cast<int>(pEntry - g_trackEntries);
        if (entryIndex >= arraysize(g_trackEntries)) // If not found, skip this.
        {
            continue;
        }
        GetQColorFromXmlNode(color, childNode);
        m_colorButtons[entryIndex]->SetColor(color);
    }

    XmlNodeRef othersNode = customTrackColorsNode->findChild("others");
    if (othersNode)
    {
        GetQColorFromXmlNode(color, othersNode);
        m_colorButtons[kOthersEntryIndex]->SetColor(color);
    }

    XmlNodeRef disabledNode = customTrackColorsNode->findChild("disabled");
    if (disabledNode)
    {
        GetQColorFromXmlNode(color, disabledNode);
        m_colorButtons[kDisabledEntryIndex]->SetColor(color);
    }

    XmlNodeRef mutedNode = customTrackColorsNode->findChild("muted");
    if (mutedNode)
    {
        GetQColorFromXmlNode(color, mutedNode);
        m_colorButtons[kMutedEntryIndex]->SetColor(color);
    }

    return true;
}
