/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TexturePresetSelectionWidget.h"
#include <Source/Editor/ui_TexturePresetSelectionWidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <AzFramework/StringFunc/StringFunc.h>
#include <BuilderSettings/PresetSettings.h>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;

    AZStd::string GetImageFileMask(const AZStd::string& imageFilePath)
    {
        const char FileMaskDelimiter = '_';

        //get file name
        AZStd::string fileName;
        QString lowerFileName = imageFilePath.data();
        lowerFileName = lowerFileName.toLower();
        AzFramework::StringFunc::Path::GetFileName(lowerFileName.toUtf8().constData(), fileName);

        //get the substring from last '_'
        size_t lastUnderScore = fileName.find_last_of(FileMaskDelimiter);
        if (lastUnderScore != AZStd::string::npos)
        {
            return fileName.substr(lastUnderScore);
        }

        return AZStd::string();
    }

    TexturePresetSelectionWidget::TexturePresetSelectionWidget(EditorTextureSetting& textureSetting, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::TexturePresetSelectionWidget)
        , m_textureSetting(&textureSetting)
    {
        m_ui->setupUi(this);

        // Add presets into combo box
        m_presetList.clear();
        auto& presetFilterMap = BuilderSettingManager::Instance()->GetPresetFilterMap();

        if (m_listAllPresets)
        {
            m_presetList = BuilderSettingManager::Instance()->GetFullPresetList();
        }
        else
        {
            auto fileMask = GetImageFileMask(m_textureSetting->m_textureName);
            auto itr = presetFilterMap.find(fileMask);
            if (itr != presetFilterMap.end())
            {
                m_presetList = itr->second;
            }
            else
            {
                m_presetList = BuilderSettingManager::Instance()->GetFullPresetList();
            }
        }

        QStringList stringList;
        foreach (const auto& presetName, m_presetList)
        {
            stringList.append(QString(presetName.GetCStr()));
        }
        stringList.sort();
        m_ui->presetComboBox->addItems(stringList);

        // Set current preset
        const auto& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
        const PresetSettings* presetSetting = BuilderSettingManager::Instance()->GetPreset(currPreset);

        if (presetSetting)
        {
            m_ui->presetComboBox->setCurrentText(presetSetting->m_name.GetCStr());
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
        m_textureSetting->SetToPreset(PresetName(m_ui->presetComboBox->currentText().toUtf8().data()));
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, true, BuilderSettingManager::s_defaultPlatform);
    }

    void TexturePresetSelectionWidget::OnChangePreset(int index)
    {
        QString text = m_ui->presetComboBox->itemText(index);
        m_textureSetting->SetToPreset(PresetName(text.toUtf8().data()));
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, true, BuilderSettingManager::s_defaultPlatform);
    }

    void ImageProcessingAtomEditor::TexturePresetSelectionWidget::OnPresetInfoButton()
    {
        const auto& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
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
            const auto& currPreset = m_textureSetting->GetMultiplatformTextureSetting().m_preset;
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

    void ImageProcessingAtomEditor::TexturePresetSelectionWidget::SetPresetConvention(const PresetSettings* presetSettings)
    {
        AZStd::string conventionText = "";
        if (presetSettings)
        {
            auto fileMasks = BuilderSettingManager::Instance()->GetFileMasksForPreset(presetSettings->m_name);
            int i = 0;
            for (const auto& filemask : fileMasks)
            {
                conventionText +=  i > 0 ? " " + filemask : filemask;
                i++;
            }
        }
        m_ui->conventionLabel->setText(QString(conventionText.c_str()));
    }

    void ImageProcessingAtomEditor::TexturePresetSelectionWidget::SetCheckBoxReadOnly(QCheckBox* checkBox, bool readOnly)
    {
        checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, readOnly);
        checkBox->setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
        checkBox->setEnabled(!readOnly);
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_TexturePresetSelectionWidget.cpp>
