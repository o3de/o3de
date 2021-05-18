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

#include "EditorDefs.h"

#include "AlembicCompileDialog.h"

// Qt
#include <QPushButton>

// AzCore
#include <AzCore/Utils/Utils.h>

#include <Pak/CryPakUtils.h>

// Editor
#include "Util/PathUtil.h"
#include "Util/EditorUtils.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <Alembic/ui_AlembicCompileDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

CAlembicCompileDialog::CAlembicCompileDialog(const XmlNodeRef config)
    : m_ui(new Ui::AlembicCompileDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_config = LoadConfig("", config);

    OnInitDialog();

    connect(m_ui->m_yUpRadio, &QRadioButton::clicked, this, &CAlembicCompileDialog::OnRadioYUp);
    connect(m_ui->m_zUpRadio, &QRadioButton::clicked, this, &CAlembicCompileDialog::OnRadioZUp);
    connect(m_ui->m_playbackFromMemoryCheckBox, &QCheckBox::clicked, this, &CAlembicCompileDialog::OnPlaybackFromMemory);
    connect(m_ui->m_blockCompressionFormatCombo, &QComboBox::currentTextChanged, this, &CAlembicCompileDialog::OnBlockCompressionSelected);
    connect(m_ui->m_meshPredictionCheckBox, &QCheckBox::clicked, this, &CAlembicCompileDialog::OnMeshPredictionCheckBox);
    connect(m_ui->m_useBFramesCheckBox, &QCheckBox::clicked, this, &CAlembicCompileDialog::OnUseBFramesCheckBox);
    connect(m_ui->m_indexFrameDistanceEdit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CAlembicCompileDialog::OnIndexFrameDistanceChanged);
    connect(m_ui->m_positionPrecisionEdit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &CAlembicCompileDialog::OnPositionPrecisionChanged);
    connect(m_ui->m_uvMaxEdit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &CAlembicCompileDialog::OnUVmaxChanged);
    connect(m_ui->m_presetComboBox, &QComboBox::currentTextChanged, this, &CAlembicCompileDialog::OnPresetSelected);
}

CAlembicCompileDialog::~CAlembicCompileDialog()
{
}

void CAlembicCompileDialog::OnInitDialog()
{
    // custom 'Ok' and 'Cancel' text for this dialog
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Recompile .cax File");
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Use Existing .cax File");

    m_ui->m_blockCompressionFormatCombo->addItem(QStringLiteral("store"));
    m_ui->m_blockCompressionFormatCombo->addItem(QStringLiteral("deflate"));
    m_ui->m_blockCompressionFormatCombo->addItem(QStringLiteral("lz4hc"));
    m_ui->m_blockCompressionFormatCombo->addItem(QStringLiteral("zstd"));
    AZStd::vector<AZStd::string> presetFiles;
    const char* const filePattern = "*.cbc";

    SDirectoryEnumeratorHelper dirHelper;

    auto engineAssetSourceRoot = AZ::IO::FixedMaxPath(AZ::Utils::GetEnginePath()) / "Assets";
    dirHelper.ScanDirectoryRecursive(gEnv->pCryPak, engineAssetSourceRoot.c_str(), "Editor/Presets/GeomCache", filePattern, presetFiles);

    for (auto iter = presetFiles.begin(); iter != presetFiles.end(); ++iter)
    {
        const auto& file = *iter;
        const AZ::IO::FixedMaxPath filePath = engineAssetSourceRoot / file;
        m_presets.push_back(LoadConfig(Path::GetFileName(file.c_str()), XmlHelpers::LoadXmlFromFile(filePath.c_str())));
        m_ui->m_presetComboBox->addItem(m_presets.back().m_name);
    }

    m_ui->m_presetComboBox->addItem(tr("(Custom)"));

    SetFromConfig(m_config);
    UpdatePresetSelection();
    UpdateEnabledStates();
}

void CAlembicCompileDialog::SetFromConfig(const SConfig& config)
{
    if (QString::compare(config.m_blockCompressionFormat, QLatin1String("deflate"), Qt::CaseInsensitive) == 0)
    {
        m_ui->m_blockCompressionFormatCombo->setCurrentIndex(1);
    }
    else if (QString::compare(config.m_blockCompressionFormat, QLatin1String("lz4hc"), Qt::CaseInsensitive) == 0)
    {
        m_ui->m_blockCompressionFormatCombo->setCurrentIndex(2);
    }
    else if (QString::compare(config.m_blockCompressionFormat, QLatin1String("zstd"), Qt::CaseInsensitive) == 0)
    {
        m_ui->m_blockCompressionFormatCombo->setCurrentIndex(3);
    }
    else
    {
        m_ui->m_blockCompressionFormatCombo->setCurrentIndex(0);
    }

    if (QString::compare(config.m_upAxis, QLatin1String("Y"), Qt::CaseInsensitive) == 0)
    {
        m_ui->m_yUpRadio->setChecked(true);
    }
    else
    {
        m_ui->m_zUpRadio->setChecked(true);
    }

    m_ui->m_playbackFromMemoryCheckBox->setChecked(config.m_playbackFromMemory == QStringLiteral("1"));
    m_ui->m_meshPredictionCheckBox->setChecked(config.m_meshPrediction == QStringLiteral("1"));
    m_ui->m_useBFramesCheckBox->setChecked(config.m_useBFrames == QStringLiteral("1"));

    m_ui->m_indexFrameDistanceEdit->setValue(config.m_indexFrameDistance);
    m_ui->m_positionPrecisionEdit->setValue(aznumeric_cast<int>(config.m_positionPrecision));
    m_ui->m_uvMaxEdit->setValue(config.m_uvMax);
}

void CAlembicCompileDialog::UpdateEnabledStates()
{
    m_ui->m_meshPredictionCheckBox->setEnabled(false);
    m_ui->m_useBFramesCheckBox->setEnabled(false);
    m_ui->m_indexFrameDistanceEdit->setEnabled(false);

    if (QString::compare(m_config.m_blockCompressionFormat, QLatin1String("store"), Qt::CaseInsensitive) != 0)
    {
        m_ui->m_meshPredictionCheckBox->setEnabled(true);
        m_ui->m_useBFramesCheckBox->setEnabled(true);

        m_ui->m_indexFrameDistanceEdit->setEnabled(m_config.m_useBFrames == QStringLiteral("1"));
    }
}

void CAlembicCompileDialog::UpdatePresetSelection()
{
    for (uint i = 0; i < m_presets.size(); ++i)
    {
        if (m_presets[i] == m_config)
        {
            m_ui->m_presetComboBox->setCurrentIndex(i);
            return;
        }
    }

    m_ui->m_presetComboBox->setCurrentIndex(m_presets.size());
}

QString CAlembicCompileDialog::GetUpAxis() const
{
    return m_config.m_upAxis;
}

QString CAlembicCompileDialog::GetPlaybackFromMemory() const
{
    return m_config.m_playbackFromMemory;
}

QString CAlembicCompileDialog::GetBlockCompressionFormat() const
{
    return m_config.m_blockCompressionFormat;
}

QString CAlembicCompileDialog::GetMeshPrediction() const
{
    return m_config.m_meshPrediction;
}

QString CAlembicCompileDialog::GetUseBFrames() const
{
    return m_config.m_useBFrames;
}

uint CAlembicCompileDialog::GetIndexFrameDistance() const
{
    return m_config.m_indexFrameDistance;
}

double CAlembicCompileDialog::GetPositionPrecision() const
{
    return m_config.m_positionPrecision;
}

float CAlembicCompileDialog::GetUVmax() const
{
    return m_config.m_uvMax;
}


void CAlembicCompileDialog::OnRadioYUp()
{
    m_config.m_upAxis = "Y";
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnRadioZUp()
{
    m_config.m_upAxis = "Z";
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnPlaybackFromMemory()
{
    m_config.m_playbackFromMemory = m_ui->m_playbackFromMemoryCheckBox->isChecked() ? QStringLiteral("1") : QStringLiteral("0");
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnBlockCompressionSelected()
{
    m_config.m_blockCompressionFormat = m_ui->m_blockCompressionFormatCombo->currentText();
    UpdatePresetSelection();
    UpdateEnabledStates();
}

void CAlembicCompileDialog::OnMeshPredictionCheckBox()
{
    m_config.m_meshPrediction = m_ui->m_meshPredictionCheckBox->isChecked() ? QStringLiteral("1") : QStringLiteral("0");
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnUseBFramesCheckBox()
{
    m_config.m_useBFrames = m_ui->m_useBFramesCheckBox->isChecked() ? QStringLiteral("1") : QStringLiteral("0");
    UpdatePresetSelection();
    UpdateEnabledStates();
}

void CAlembicCompileDialog::OnIndexFrameDistanceChanged()
{
    m_config.m_indexFrameDistance = m_ui->m_indexFrameDistanceEdit->value();
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnPositionPrecisionChanged()
{
    m_config.m_positionPrecision = m_ui->m_positionPrecisionEdit->value();
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnUVmaxChanged()
{
    m_config.m_uvMax = aznumeric_cast<float>(m_ui->m_uvMaxEdit->value());
    UpdatePresetSelection();
}

void CAlembicCompileDialog::OnPresetSelected()
{
    uint presetIndex = m_ui->m_presetComboBox->currentIndex();
    if (presetIndex < m_presets.size())
    {
        m_config = m_presets[presetIndex];
        SetFromConfig(m_config);
    }

    m_ui->m_presetComboBox->setCurrentIndex(presetIndex);
    UpdateEnabledStates();
}

CAlembicCompileDialog::SConfig CAlembicCompileDialog::LoadConfig(const QString& fileName, XmlNodeRef xml) const
{
    SConfig config;
    config.m_name = fileName;

    if (xml)
    {
        config.m_name = xml->getAttr("Name");
        config.m_blockCompressionFormat = xml->getAttr("BlockCompressionFormat");
        config.m_upAxis = xml->getAttr("UpAxis");
        config.m_playbackFromMemory = xml->getAttr("PlaybackFromMemory");
        config.m_meshPrediction = xml->getAttr("MeshPrediction");
        config.m_useBFrames = xml->getAttr("UseBFrames");
        xml->getAttr("IndexFrameDistance", config.m_indexFrameDistance);
        xml->getAttr("PositionPrecision", config.m_positionPrecision);
        xml->getAttr("UVmax", config.m_uvMax);
    }

    return config;
}

#include <Alembic/moc_AlembicCompileDialog.cpp>
