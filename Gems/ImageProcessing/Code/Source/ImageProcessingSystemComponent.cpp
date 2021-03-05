/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ImageProcessing_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <Editor/TexturePropertyEditor.h>
#include <Processing/ImagePreview.h>

#include "ImageProcessingSystemComponent.h"
#include <QMenu>
#include <QMessageBox>

namespace ImageProcessing
{
    void ImageProcessingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ImageProcessingSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void ImageProcessingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ImageBuilderService", 0x43c4be37));
    }

    void ImageProcessingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ImageBuilderService", 0x43c4be37));
    }

    void ImageProcessingSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ImageProcessingSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ImageProcessingSystemComponent::Init()
    {

    }

    void ImageProcessingSystemComponent::Activate()
    {
        // Call to allocate BuilderSettingManager
        BuilderSettingManager::CreateInstance();

        ImageProcessingEditor::ImageProcessingEditorRequestBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler::BusConnect();
    }

    void ImageProcessingSystemComponent::Deactivate()
    {
        ImageProcessingEditor::ImageProcessingEditorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::Handler::BusDisconnect();

        // Deallocate BuilderSettingManager
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
    }
     
    void ImageProcessingSystemComponent::OpenSourceTextureFile(const AZ::Uuid& textureSourceID)
    {
        if (textureSourceID.IsNull())
        {
            QMessageBox::warning(QApplication::activeWindow(), "Warning",
                "Texture source does not have a unique ID. This can occur if the source asset has not yet been processed by the Asset Processor.",
                QMessageBox::Ok);
        }
        else
        {
            ImageProcessingEditor::TexturePropertyEditor editor(textureSourceID, QApplication::activeWindow());
            if (!editor.HasValidImage())
            {
                QMessageBox::warning(QApplication::activeWindow(), "Warning", "Invalid texture file", QMessageBox::Ok);
                return;
            }
            editor.exec();
        }
    }

    void ImageProcessingSystemComponent::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        if (HandlesSource(fullSourceFileName))
        {
            openers.push_back(
                {
                    "Image_Processing_Editor",
                    "Edit Image Settings...",
                    QIcon(),
                [&](const char* fullSourceFileNameInCallback, const AZ::Uuid& sourceUUID)
                    {
                        AZ_UNUSED(fullSourceFileNameInCallback);

                        if (!LoadTextureSettings())
                        {
                            return;
                        }

                        ImageProcessingEditor::ImageProcessingEditorRequestBus::Broadcast(&ImageProcessingEditor::ImageProcessingEditorRequests::OpenSourceTextureFile, sourceUUID);
                    }
                });
        }
    }

    bool ImageProcessingSystemComponent::HandlesSource(AZStd::string_view fileName) const
    {
        for (int i = 0; i < s_TotalSupportedImageExtensions; i ++ )
        {
            if (AZStd::wildcard_match(s_SupportedImageExtensions[i], fileName.data()))
            {
                return true;
            }
        }

        return false;
    }

    bool ImageProcessingSystemComponent::GetProductTexturePreview(const char* fullProductFileName, QImage& previewImage, AZStd::string& productInfo, AZStd::string& productAlphaInfo)
    {
        return ImagePreview::GetProductTexturePreview(fullProductFileName, previewImage, productInfo, productAlphaInfo);
    }

    bool ImageProcessingSystemComponent::LoadTextureSettings()
    {
        if (m_textureSettingsLoaded)
        {
            return true;
        }

        // Load the preset settings before opening the editor
        auto outcome = BuilderSettingManager::Instance()->LoadBuilderSettings();
        if (outcome.IsSuccess())
        {
            m_textureSettingsLoaded = true;
            return true;
        }

        AZ_Error("Image Processing", false, "Failed to load default preset settings!");
        return false;
    }

} // namespace ImageProcessing
