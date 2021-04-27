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

#include "ImageProcessing_precompiled.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImageToProcess.h>
#include <Processing/Utils.h>
#include <Thumbnail/ImageThumbnail.h>
#include <Thumbnail/ImageThumbnailSystemComponent.h>

namespace ImageProcessingAtom
{
    namespace Thumbnails
    {
        void ImageThumbnailSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<ImageThumbnailSystemComponent, AZ::Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<ImageThumbnailSystemComponent>("ImageThumbnailSystemComponent", "System component for image thumbnails.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                }
            }
        }

        void ImageThumbnailSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImageThumbnailSystem"));
        }

        void ImageThumbnailSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ImageThumbnailSystem"));
        }

        void ImageThumbnailSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ThumbnailerService"));
        }

        void ImageThumbnailSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void ImageThumbnailSystemComponent::Activate()
        {
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusConnect(AZ::RPI::StreamingImageAsset::RTTI_Type());
            SetupThumbnails();
        }

        void ImageThumbnailSystemComponent::Deactivate()
        {
            TeardownThumbnails();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusDisconnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        }

        void ImageThumbnailSystemComponent::SetupThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;

            ThumbnailerRequestsBus::Broadcast(
                &ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(Thumbnails::ImageThumbnailCache),
                ThumbnailContext::DefaultContext);
        }

        void ImageThumbnailSystemComponent::TeardownThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;

            ThumbnailerRequestsBus::Broadcast(
                &ThumbnailerRequests::UnregisterThumbnailProvider, Thumbnails::ImageThumbnailCache::ProviderName,
                ThumbnailContext::DefaultContext);
        }

        void ImageThumbnailSystemComponent::OnApplicationAboutToStop()
        {
            TeardownThumbnails();
        }

        bool ImageThumbnailSystemComponent::Installed() const
        {
            return true;
        }

        void ImageThumbnailSystemComponent::RenderThumbnail(
            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize)
        {
            auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(thumbnailKey.data());
            if (sourceKey)
            {
                bool foundIt = false;
                AZ::Data::AssetInfo assetInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, sourceKey->GetSourceUuid(),
                    assetInfo, watchFolder);

                if (foundIt)
                {
                    AZStd::string fullPath;
                    AZ::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), fullPath);
                    if (RenerThumbnailFromImage(thumbnailKey, thumbnailSize, IImageObjectPtr(LoadImageFromFile(fullPath))))
                    {
                        return;
                    }
                }
            }

            auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(thumbnailKey.data());
            if (productKey)
            {
                if (RenerThumbnailFromImage(thumbnailKey, thumbnailSize, Utils::LoadImageFromImageAsset(productKey->GetAssetId())))
                {
                    return;
                }
            }

            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
        }

        bool ImageThumbnailSystemComponent::RenerThumbnailFromImage(
            AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize, IImageObjectPtr previewImage) const
        {
            if (!previewImage)
            {
                return false;
            }

            ImageToProcess imageToProcess(previewImage);
            imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
            previewImage = imageToProcess.Get();

            AZ::u8* imageBuf = nullptr;
            AZ::u32 mip = 0;
            AZ::u32 pitch = 0;
            previewImage->GetImagePointer(mip, imageBuf, pitch);
            const AZ::u32 width = previewImage->GetWidth(mip);
            const AZ::u32 height = previewImage->GetHeight(mip);

            QImage image(imageBuf, width, height, pitch, QImage::Format_RGBA8888);

            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                thumbnailKey, &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                QPixmap::fromImage(image.scaled(QSize(thumbnailSize, thumbnailSize), Qt::KeepAspectRatio, Qt::SmoothTransformation)));

            return true;
        }
    } // namespace Thumbnails
} // namespace ImageProcessingAtom
