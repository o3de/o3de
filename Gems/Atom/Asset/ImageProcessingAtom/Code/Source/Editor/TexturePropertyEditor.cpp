/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TexturePropertyEditor.h"

// warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4800 4251, "-Wunknown-warning-option")
#include <Source/Editor/ui_TexturePropertyEditor.h>
AZ_POP_DISABLE_WARNING

#include <Source/BuilderSettings/BuilderSettingManager.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/IO/FileIO.h>

// warning C4251: 'QBrush::d': class 'QScopedPointer<QBrushData,QBrushDataPointerDeleter>' needs to have dll-interface to be used by clients of class 'QBrush'
// warning C4800: 'uint': forcing value to bool 'true' or 'false' (performance warning)
// warning C4244: 'argument': conversion from 'UINT64' to 'AZ::u32',
AZ_PUSH_DISABLE_WARNING(4244 4800 4251, "-Wunknown-warning-option")
#include <QBoxLayout>
#include <QUrl>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QPushButton>

#include <QCheckBox>
#include <QComboBox>
AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtomEditor
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
        AZStd::string outputPath = m_textureSetting.m_fullPath + ImageProcessingAtom::TextureSettings::ExtensionName;

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

        ImageProcessingAtom::TextureSettings& baseSetting = m_textureSetting.GetMultiplatformTextureSetting();
        for (auto& it : m_textureSetting.m_settingsMap)
        {
            baseSetting.ApplySettings(it.second, it.first);
        }

        ImageProcessingAtom::StringOutcome outcome = ImageProcessingAtom::TextureSettings::WriteTextureSetting(outputPath, baseSetting);

        if (!outcome.IsSuccess())
        {
            AZ_Error("Texture Editor", false, "Cannot save texture settings to %s!", outputPath.data());
        }
    }
    
    void TexturePropertyEditor::OnHelp()
    {
        QString webLink = tr("https://o3de.org/docs/");
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
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_TexturePropertyEditor.cpp>
