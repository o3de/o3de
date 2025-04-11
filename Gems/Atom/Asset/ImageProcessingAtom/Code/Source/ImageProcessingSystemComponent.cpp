/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/wildcard.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <BuilderSettings/BuilderSettingManager.h>
#include <Editor/TexturePropertyEditor.h>

#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImagePreview.h>
#include <Processing/ImageToProcess.h>
#include <Processing/Utils.h>

#include "ImageProcessingSystemComponent.h"
#include <QFileDialog>
#include <QMenu>

namespace ImageProcessingAtom
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
        provided.push_back(AZ_CRC_CE("AtomImageBuilderService"));
    }

    void ImageProcessingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomImageBuilderService"));
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
        m_previewerFactory.reset(new ImagePreviewerFactory);
    }

    void ImageProcessingSystemComponent::Activate()
    {
        // Note: the editor initialization will only report incompatible components if we have two system components are incompatible.
        // It won't interrupt the initialization. And the CSystem::Init would continue and report totally irrelevant message 
        // due to cinematic system pointer was empty since the ActivateEntities returned early
        // This is a temporary solution until we have the proper system for gem incompatible mechanism in place ( LY-105408)
        // Here it would pop out a message box if we found the LY ImageProcessingSystemComponent was reflected. 
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext->FindClassData(AZ::Uuid("{13B1EB88-316F-4D44-B59C-886F023A5A58}")))
        {
            AZ::OSString errorMsg = "Incompatible gem detected. Please disable ImageProcessing Gem for Atom project";
            AZ_Error("ImageProcessingAtom", false, errorMsg.data());
            AZ::NativeUI::NativeUIRequestBus::Broadcast(&AZ::NativeUI::NativeUIRequestBus::Events::DisplayOkDialog, "", errorMsg.c_str(), false);
            return;
        }

        // Call to allocate BuilderSettingManager
        BuilderSettingManager::CreateInstance();

        ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusConnect();

        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusConnect();
        ImageProcessingRequestBus::Handler::BusConnect();
    }

    void ImageProcessingSystemComponent::Deactivate()
    {
        ImageProcessingRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler::BusDisconnect();
        ImageProcessingAtomEditor::ImageProcessingEditorRequestBus::Handler::BusDisconnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();

        // Deallocate BuilderSettingManager
        BuilderSettingManager::DestroyInstance();
        CPixelFormats::DestroyInstance();
    }

    void ImageProcessingSystemComponent::OpenSourceTextureFile(const AZ::Uuid& textureSourceID)
    {
        ImageProcessingAtomEditor::TexturePropertyEditor editor(textureSourceID, QApplication::activeWindow());
        editor.exec();
    }

    IImageObjectPtr ImageProcessingSystemComponent::LoadImage(const AZStd::string& filePath)
    {
        return IImageObjectPtr(LoadImageFromFile(filePath));
    }

    IImageObjectPtr ImageProcessingSystemComponent::LoadImagePreview(const AZStd::string& filePath)
    {
        IImageObjectPtr image(LoadImageFromFile(filePath));
        if (image)
        {
            ImageToProcess imageToProcess(image);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
            return imageToProcess.Get();
        }
        return image;
    }

    void ImageProcessingSystemComponent::AddContextMenuActions(QWidget* /*caller*/, QMenu* menu, const AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries)
    {
        // Load Texture Settings
        static bool isSettingLoaded = false;

        if (!isSettingLoaded)
        {
            // Load the preset settings before editor open
            auto outcome = ImageProcessingAtom::BuilderSettingManager::Instance()->LoadConfig();

            if (outcome.IsSuccess())
            {
                isSettingLoaded = true;
            }
            else
            {
                AZ_Error("Image Processing", false, "Failed to load default preset settings!");
                return;
            }
        }

        // Register right click menu
        using namespace AzToolsFramework::AssetBrowser;
        auto entryIt = AZStd::find_if
            (
            entries.begin(),
            entries.end(),
            [](const AssetBrowserEntry* entry) -> bool
            {
                return entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source
                    || entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product;
            }
            );

        if (entryIt == entries.end())
        {
            return;
        }

        if ((*entryIt)->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
        {
            // For supported source image files, add menu item to open texture setting editor
            const SourceAssetBrowserEntry* source = azrtti_cast<const SourceAssetBrowserEntry*>(*entryIt);

            if (!HandlesSource(source))
            {
                return;
            }

            AZ::Uuid sourceId = source->GetSourceUuid();
            if (!sourceId.IsNull())
            {
                menu->addAction("Edit Texture Settings...", [sourceId, this]()
                    {
                        OpenSourceTextureFile(sourceId);
                    });
            }
        }
        else if ((*entryIt)->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
        {
            // For product which is streaming image asset, add menu item to save it to a dds file
            const ProductAssetBrowserEntry* product = azrtti_cast<const ProductAssetBrowserEntry*>(*entryIt);
            if (product->GetAssetType() == azrtti_typeid<AZ::RPI::StreamingImageAsset>())
            {
                AZ::Data::AssetId assetId = product->GetAssetId();
                menu->addAction("Save as DDS...", [assetId, this]()
                    {
                        QString filePath = AzQtComponents::FileDialog::GetSaveFileName(nullptr, QString("Save to file"), m_lastSavedPath, QString("DDS file (*.dds)"));
                        if (filePath.isEmpty())
                        {
                            return;
                        }
                        if (SaveStreamingImageAssetToDDS(assetId, filePath.toUtf8().data()))
                        {
                            AZ_Printf("Image Processing", "Image was saved to a dds file %s", filePath.toUtf8().data());
                            m_lastSavedPath = filePath;
                        }
                    });
            }
        }
    }

    bool ImageProcessingSystemComponent::HandlesSource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* entry) const
    {
        AZStd::string targetExtension = entry->GetExtension();

        for (int i = 0; i < s_TotalSupportedImageExtensions; i++)
        {
            if (AZStd::wildcard_match(s_SupportedImageExtensions[i], targetExtension.c_str()))
            {
                return true;
            }
        }

        return false;
    }

    const AzToolsFramework::AssetBrowser::PreviewerFactory* ImageProcessingSystemComponent::GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const
    {
        return m_previewerFactory->IsEntrySupported(entry) ? m_previewerFactory.get() : nullptr;
    }

    bool ImageProcessingSystemComponent::SaveStreamingImageAssetToDDS(const AZ::Data::AssetId& assetId, AZStd::string_view filePath)
    {
        IImageObjectPtr loadedImage = Utils::LoadImageFromImageAsset(assetId);
        if (!loadedImage)
        {
            AZ_Warning("ImageProcessingSystemComponent", false, "Failed to load product asset");
            return false;
        }

        if (!Utils::SaveImageToDdsFile(loadedImage, filePath))
        {
            AZ_Warning("ImageProcessingSystemComponent", false, "Failed to save image to dds file %s", filePath.data());
            return false;
        }
        return true;
    }
} // namespace ImageProcessingAtom
