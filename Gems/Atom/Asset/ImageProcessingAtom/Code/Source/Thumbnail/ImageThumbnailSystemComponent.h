/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace ImageProcessingAtom
{
    namespace Utils
    {
        class AsyncImageAssetLoader;
    }

    namespace Thumbnails
    {
        //! System component for image thumbnails.
        class ImageThumbnailSystemComponent
            : public AZ::Component
            , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
            , public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ImageThumbnailSystemComponent, "{C45D69BB-4A3B-49CF-916B-580F05CAA755}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Activate() override;
            void Deactivate() override;

        private:
            void SetupThumbnails();
            void TeardownThumbnails();

            // AzFramework::ApplicationLifecycleEvents overrides...
            void OnApplicationAboutToStop() override;

            // ThumbnailerRendererRequestBus::Handler interface overrides...
            bool Installed() const override;
            void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;

            template<class MkImageFn>
            void RenderThumbnailFromImage(
                AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize, MkImageFn mkPreviewImage) const;

            AZStd::shared_ptr<Utils::AsyncImageAssetLoader> m_imageAssetLoader;
        };
    } // namespace Thumbnails
} // namespace ImageProcessingAtom
