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
#include "TexturePropertyEditor.h"
#include <Source/Editor/ui_TexturePropertyEditor.h>

#include <Source/BuilderSettings/BuilderSettingManager.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/IO/FileIO.h>

#include <QBoxLayout>
#include <QUrl>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QPushButton>

#include <QCheckBox>
#include <QComboBox>

namespace ImageProcessingEditor
{
    TexturePropertyEditor::TexturePropertyEditor(const AZ::Uuid& sourceTextureId, QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
        , m_ui(new Ui::TexturePropertyEditor)
        , m_textureSetting(sourceTextureId)
        , m_validImage(true)
    {
        if (m_textureSetting.m_img == nullptr)
        {
            m_validImage = false;
            return;
        }

        m_ui->setupUi(this);

        //Initialize all the format string here
        EditorHelper::InitPixelFormatString();

        //TexturePreviewWidget will be the widget to preview mipmaps
        m_previewWidget.reset(aznew TexturePreviewWidget(m_textureSetting, this));
        m_ui->mainLayout->layout()->addWidget(m_previewWidget.data());

        //TexturePresetSelectionWidget will be the widget to select the preset for the texture
        m_presetSelectionWidget.reset(aznew TexturePresetSelectionWidget(m_textureSetting, this));
        m_ui->mainLayout->layout()->addWidget(m_presetSelectionWidget.data());

        //ResolutionSettingWidget will be the table section to display mipmap resolution for each platform
        m_resolutionSettingWidget.reset(aznew ResolutionSettingWidget(ResoultionWidgetType::TexturePropety, m_textureSetting, this));
        m_ui->mainLayout->layout()->addWidget(m_resolutionSettingWidget.data());

        //MipmapSettingWidget will be simple ReflectedProperty editor to reflect mipmap settings section
        m_mipmapSettingWidget.reset(aznew MipmapSettingWidget(m_textureSetting, this));
        m_ui->mainLayout->layout()->addWidget(m_mipmapSettingWidget.data());

        // Disable horizontal scroll
        m_ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QObject::connect(m_ui->saveBtn, &QPushButton::clicked, this, &TexturePropertyEditor::OnSave);
        QObject::connect(m_ui->helpBtn, &QPushButton::clicked, this, &TexturePropertyEditor::OnHelp);
        QObject::connect(m_ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

        EditorInternalNotificationBus::Handler::BusConnect();

        // When checkbox and combobox is focused, they will intercept the space shortcut, need to disable focus on them first
        // to get space shortcut pass through
        QList<QCheckBox*> checkBoxWidgets = QObject::findChildren<QCheckBox*>();
        for (QCheckBox* widget: checkBoxWidgets)
        {
            widget->setFocusPolicy(Qt::NoFocus);
        }
        QList<QComboBox*> comboBoxWidgets = QObject::findChildren<QComboBox*>();
        for (QComboBox* widget : comboBoxWidgets)
        {
            widget->setFocusPolicy(Qt::NoFocus);
        }
        this->setFocusPolicy(Qt::StrongFocus);

    }

    TexturePropertyEditor::~TexturePropertyEditor()
    {
        EditorInternalNotificationBus::Handler::BusDisconnect();
    }

    bool TexturePropertyEditor::HasValidImage()
    {
        return m_validImage;
    }

    void TexturePropertyEditor::OnEditorSettingsChanged([[maybe_unused]] bool needRefresh, const AZStd::string& /*platform*/)
    {
        m_textureSetting.m_modified = true;
    }

    void TexturePropertyEditor::OnSave()
    {
        if (!m_validImage)
        {
            return;
        }

        bool sourceControlActive = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive, &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        AZStd::string outputPath = m_textureSetting.m_fullPath + ImageProcessing::TextureSettings::modernExtensionName;

        if (sourceControlActive)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, outputPath.c_str(), "Checking out .imagesetting file", []([[maybe_unused]] int& current, [[maybe_unused]] int& max) {});

            if (checkoutResult)
            {
                SaveTextureSetting(outputPath);
            }
            else
            {
                AZ_Error("Texture Editor", false, "Cannot checkout file '%s' from source control.", outputPath.c_str());
            }
        }
        else
        {
            const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(outputPath.c_str());
            const bool fileReadOnly = AZ::IO::FileIOBase::GetInstance()->IsReadOnly(outputPath.c_str());

            if (!fileExisted || !fileReadOnly)
            {
                SaveTextureSetting(outputPath);
            }
        }
    }

    void TexturePropertyEditor::SaveTextureSetting(AZStd::string outputPath)
    {
        if (!m_validImage)
        {
            return;
        }

        ImageProcessing::TextureSettings& baseSetting = m_textureSetting.GetMultiplatformTextureSetting();
        for (auto& it : m_textureSetting.m_settingsMap)
        {
            baseSetting.ApplySettings(it.second, it.first);
        }
            
        ImageProcessing::StringOutcome outcome = ImageProcessing::TextureSettings::WriteTextureSetting(outputPath, baseSetting);   

        if (outcome.IsSuccess())
        {
            // Since setting is successfully saved, we can safely delete the legacy setting now
            DeleteLegacySetting();
        }
        else
        {
            AZ_Error("Texture Editor", false, "Cannot save texture settings!");
        }
    }

    void TexturePropertyEditor::DeleteLegacySetting()
    {
        AZStd::string legacyFile = m_textureSetting.m_fullPath + ImageProcessing::TextureSettings::legacyExtensionName;
        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(legacyFile.c_str());
        if (fileExisted)
        {
            bool sourceControlActive = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive, &AzToolsFramework::SourceControlConnectionRequests::IsActive);

            if (sourceControlActive)
            {
                bool checkoutResult = false;

                AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestDelete, legacyFile.c_str(),
                    [](bool success, const AzToolsFramework::SourceControlFileInfo& info)
                {
                    //Deletes the file locally if it's not tracked by source control
                    if (!success && !info.IsManaged())
                    {
                        AZ::IO::FileIOBase::GetInstance()->Remove(info.m_filePath.c_str());
                    }
                });
            }
            else
            {
                AZ::IO::FileIOBase::GetInstance()->Remove(legacyFile.c_str());
            }
        }
    }


    void TexturePropertyEditor::OnHelp()
    {
        QString webLink = tr("https://docs.aws.amazon.com/console/lumberyard/texturepipeline");
        QDesktopServices::openUrl(QUrl(webLink));
    }


    bool TexturePropertyEditor::event(QEvent* event)
    {   
        bool needsBlocking = false;
        if (m_previewWidget)
        {
            needsBlocking = m_previewWidget->OnQtEvent(event);
        }
        return needsBlocking ? true : QWidget::event(event);
    }
    
}//namespace ImageProcessingEditor
#include <Source/Editor/moc_TexturePropertyEditor.cpp>
