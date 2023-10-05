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
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <Atom/RPI.Public/AssetTagBus.h>

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
        m_presetList = BuilderSettingManager::Instance()->GetFullPresetList();

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
        }

        // Reset btn
        QObject::connect(m_ui->resetBtn, &QPushButton::clicked, this, &TexturePresetSelectionWidget::OnRestButton);

        // PresetInfo btn
        QObject::connect(m_ui->infoBtn, &QPushButton::clicked, this, &TexturePresetSelectionWidget::OnPresetInfoButton);

        // Style the Checkbox
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_ui->serCheckBox);

        // Tooltips
        m_ui->activePresetLabel->setToolTip(QString("Choose a texture preset based on the texture's intended use case."));
        m_ui->infoBtn->setToolTip(QString("Display the property details for the selected texture preset"));
        m_ui->resetBtn->setToolTip(QString("Reset all values to the default values for the selected texture preset."));
        m_ui->maxResLabel->setToolTip(QString("Use the maximum texture resolution regardless of the target platform specification. Use this setting for textures that feature text or other details that should be legible."));

        AZStd::vector<AZ::Name> textureTags;
        AZ::RPI::ImageTagBus::BroadcastResult(textureTags, &AZ::RPI::ImageTagBus::Events::GetTags);

        for (const AZ::Name& tag : textureTags)
        {
            m_ui->tagComboBox->addItem(tag.GetCStr());
        }

        m_ui->tagList->setSortingEnabled(true);

        for (const AZStd::string& tag : m_textureSetting->GetMultiplatformTextureSetting().m_tags)
        {
            m_ui->tagList->addItem(QString(tag.c_str()));
        }

        QObject::connect(m_ui->tagAddButton, &QPushButton::released, this, &TexturePresetSelectionWidget::OnTagAdded);
        QObject::connect(m_ui->tagRemoveButton, &QPushButton::released, this, &TexturePresetSelectionWidget::OnTagRemoved);

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

    void TexturePresetSelectionWidget::OnTagAdded()
    {
        QString selectedTag = m_ui->tagComboBox->currentText();
        if (selectedTag.isEmpty())
        {
            return;
        }

        auto& tags = m_textureSetting->GetMultiplatformTextureSetting().m_tags;
        if (!tags.emplace(selectedTag.toUtf8().data()).second)
        {
            return;
        }

        m_ui->tagList->addItem(selectedTag);
    }

    void TexturePresetSelectionWidget::OnTagRemoved()
    {
        QListWidgetItem* item = m_ui->tagList->currentItem();
        if (!item)
        {
            return;
        }

        auto& tags = m_textureSetting->GetMultiplatformTextureSetting().m_tags;
        tags.erase(item->text().toUtf8().data());

        delete item;
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
                // If there is preset info dialog open, update the text
                if (m_presetPopup && m_presetPopup->isVisible())
                {
                    m_presetPopup->RefreshPresetInfoLabel(presetSetting);
                }
            }
            m_ui->serCheckBox->blockSignals(oldState);
        }
    }

    void ImageProcessingAtomEditor::TexturePresetSelectionWidget::SetCheckBoxReadOnly(QCheckBox* checkBox, bool readOnly)
    {
        checkBox->setAttribute(Qt::WA_TransparentForMouseEvents, readOnly);
        checkBox->setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
        checkBox->setEnabled(!readOnly);
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_TexturePresetSelectionWidget.cpp>
