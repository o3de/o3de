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

namespace OpenParticleSystem
{
    namespace Thumbnails
    {
        class ParticleMaterialThumbnailSystemComponent
            : public AZ::Component
            , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
            , public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ParticleMaterialThumbnailSystemComponent, "{0532401C-3FAE-DFF8-1AEE-BB0769005CA6}");

            // AZ::Component interface overrides
            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        protected:
            void Activate() override;
            void Deactivate() override;

        private:
            void SetupThumbnails();
            void TeardownThumbnails();

            // AzFramework::ApplicationLifecycleEvents overrides
            void OnApplicationAboutToStop() override;

            // ThumbnailerRendererRequestBus::Handler interface overrides
            bool Installed() const override;
            void RenderThumbnail([[maybe_unused]]AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, [[maybe_unused]]int thumbnailSize) override;
        };
    } // namespace Thumbnails
} // namespace OpenParticleSystem
