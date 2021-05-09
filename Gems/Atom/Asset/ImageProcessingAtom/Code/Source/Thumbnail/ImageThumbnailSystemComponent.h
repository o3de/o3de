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
#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace ImageProcessingAtom
{
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

            // ThumbnailerRendererRequestsBus::Handler interface overrides...
            bool Installed() const override;
            void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;

            bool RenderThumbnailFromImage(
                AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize, IImageObjectPtr previewImage) const;
        };
    } // namespace Thumbnails
} // namespace ImageProcessingAtom
