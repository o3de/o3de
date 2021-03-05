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

#include <ImageProcessing_precompiled.h>
#include "TexturePresetSelectionWidget.h"
#include <Source/Editor/ui_TexturePresetSelectionWidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <AzFramework/StringFunc/StringFunc.h>
#include <BuilderSettings/PresetSettings.h>

namespace ImageProcessingEditor
{
    using namespace ImageProcessing;
    TexturePresetSelectionWidget::TexturePresetSelectionWidget(EditorTextureSetting& textureSetting, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::TexturePresetSelectionWidget)
        , m_textureSetting(&textureSetting)
    {
        m_ui->setupUi(this);

        // Add presets into combo box
        m_presetList.clear();
        auto& presetFilterMap = BuilderSettingManager::Instance()->GetPresetFilterMap();

        AZStd::set<AZStd::string> noFilterPresetList;

        // Check if there is any filtered preset list first
        for(auto& presetFilter : presetFilterMap)
        {
            if (presetFilter.first.empty())
            {
                noFilterPresetList = presetFilter.second;
            }
            else if (IsMatchingWithFileMask(m_textureSetting->m_textureName, presetFilter.first))
            {
                for(const AZStd::string& presetName : presetFilter.second)
                {
                    m_presetList.insert(presetName);
                }
            }
        }
        // If no filtered preset list available or should list all presets, use non-filter list
        if (m_presetList.size() == 0 || m_listAllPresets)
        {
            m_presetList = noFilterPresetList;
        }

        foreach (const AZStd::string& presetName, m_presetList)
        {
            m_ui->presetComboBox->addItem(QString(presetName.c_str()));
        }
        
        // Set current preset
        const AZ::Uuid& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(currPreset);

        if (presetSetting)
        {
            m_ui->presetComboBox->setCurrentText(presetSetting->m_name.c_str());
            QObject::connect(m_ui->presetComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TexturePresetSelectionWidget::OnChangePreset);

            // Suppress engine reduction checkbox
            m_ui->serCheckBox->setCheckState(m_textureSetting->GetMultiplatformTextureSetting().m_suppressEngineReduce ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

            SetCheckBoxReadOnly(m_ui->serCheckBox, presetSetting->m_suppressEngineReduce);
            QObject::connect(m_ui->serCheckBox, &QCheckBox::clicked, this, &TexturePresetSelectionWidget::OnCheckBoxStateChanged);

            // Set convention label
            SetPresetConvention(presetSetting);
        }

        // Reset btn
        QObject::connect(m_ui->resetBtn, &QPushButton::clicked, this, &TexturePresetSelectionWidget::OnRestButton);

        // PresetInfo btn
        QObject::connect(m_ui->infoBtn, &QPushButton::clicked, this, &TexturePresetSelectionWidget::OnPresetInfoButton);

        // Tooltips
        m_ui->activeFileConventionLabel->setToolTip(QString("Displays the supported naming convention for the selected preset."));
        m_ui->presetComboBox->setToolTip(QString("Choose a preset to update the preview and other properties."));
        m_ui->resetBtn->setToolTip(QString("Reset values to current preset defaults."));
        m_ui->serCheckBox->setToolTip(QString("Preserves the original size. Use this setting for textures that include text."));
        m_ui->infoBtn->setToolTip(QString("Show detail properties of the current preset"));

        EditorInternalNotificationBus::Handler::BusConnect();
    }

    TexturePresetSelectionWidget::~TexturePresetSelectionWidget()
    {
        EditorInternalNotificationBus::Handler::BusDisconnect();
    }

    void TexturePresetSelectionWidget::OnCheckBoxStateChanged(bool checked)
    {
        for (auto& it : m_textureSetting->m_settingsMap)
        {
            it.second.m_suppressEngineReduce = checked;
        }
        m_textureSetting->SetIsOverrided();
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, false, BuilderSettingManager::s_defaultPlatform);
    }

    void TexturePresetSelectionWidget::OnRestButton()
    {
        m_textureSetting->SetToPreset(AZStd::string(m_ui->presetComboBox->currentText().toUtf8().data()));
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, true, BuilderSettingManager::s_defaultPlatform);
    }

    void TexturePresetSelectionWidget::OnChangePreset(int index)
    {
        QString text = m_ui->presetComboBox->itemText(index);
        m_textureSetting->SetToPreset(AZStd::string(text.toUtf8().data()));
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, true, BuilderSettingManager::s_defaultPlatform);
    }

    void ImageProcessingEditor::TexturePresetSelectionWidget::OnPresetInfoButton()
    {
        const AZ::Uuid& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(currPreset);
        m_presetPopup.reset(new PresetInfoPopup(presetSetting, this));
        m_presetPopup->installEventFilter(this);
        m_presetPopup->show();
    }

    void TexturePresetSelectionWidget::OnEditorSettingsChanged(bool needRefresh, const AZStd::string& /*platform*/)
    {
        if (needRefresh)
        {
            bool oldState = m_ui->serCheckBox->blockSignals(true);
            m_ui->serCheckBox->setChecked(m_textureSetting->GetMultiplatformTextureSetting().m_suppressEngineReduce);
            // If the preset's SER is true, texture setting should not override
            const AZ::Uuid& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
            const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(currPreset);
            if (presetSetting)
            {
                SetCheckBoxReadOnly(m_ui->serCheckBox, presetSetting->m_suppressEngineReduce);
                SetPresetConvention(presetSetting);
                // If there is preset info dialog open, update the text
                if (m_presetPopup && m_presetPopup->isVisible())
                {
                    m_presetPopup->RefreshPresetInfoLabel(presetSetting);
                }
            }
            m_ui->serCheckBox->blockSignals(oldState);
        }
    }

    bool TexturePresetSelectionWidget::IsMatchingWithFileMask(const AZStd::string& filename, const AZStd::string& fileMask)
    {
        if (fileMask.empty())
        {
            // Will not match with empty string
            return false;
        }
        else
        {
            // Extract the file name and compare if it ends with file mask or not
            AZStd::string filenameNoExt;
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), filenameNoExt);
            return filenameNoExt.length() >= fileMask.length() && filenameNoExt.compare(filenameNoExt.length() - fileMask.length(), fileMask.length(), fileMask) == 0;
        }
    }

    void ImageProcessingEditor::TexturePresetSelectionWidget::SetPresetConvention(const PresetSettings* presetSettings)
    {
        AZStd::string conventionText = "";
        if (presetSettings)
        {   
            int i = 0;
            for (const PlatformName& filemask : presetSettings->m_fileMasks)
            {   
                conventionText +=  i > 0 ? " " + filemask : filemask;
                i++;
            }
        }
        m_ui->conventionLabel->setText(QString(conventionText.c_str()));
    }

    void ImageProcessingEditor::TexturePresetSelectionWidget::SetCheckBoxReadOnly(QCheckBox* checkBox, bool readOnly)
    {
        checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, readOnly);
        checkBox->setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
        checkBox->setEnabled(!readOnly);
    }

}//namespace ImageProcessingEditor
#include <Source/Editor/moc_TexturePresetSelectionWidget.cpp>
