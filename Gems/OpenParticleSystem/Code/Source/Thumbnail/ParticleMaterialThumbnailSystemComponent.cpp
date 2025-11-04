/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Thumbnail/ParticleMaterialThumbnailSystemComponent.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <Thumbnail/ParticleMaterialThumbnail.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

namespace OpenParticleSystem
{
    namespace Thumbnails
    {
        void ParticleMaterialThumbnailSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<ParticleMaterialThumbnailSystemComponent, AZ::Component>()
                    ->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<ParticleMaterialThumbnailSystemComponent>("ParticleMaterialThumbnailSystemComponent", "System component for image thumbnails.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                }
            }
        }

        void ParticleMaterialThumbnailSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ParticleMaterialThumbnailSystem"));
        }

        void ParticleMaterialThumbnailSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ParticleMaterialThumbnailSystem"));
        }

        void ParticleMaterialThumbnailSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("ThumbnailerService"));
        }

        void ParticleMaterialThumbnailSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void ParticleMaterialThumbnailSystemComponent::Activate()
        {
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusConnect(AZ::RPI::MaterialAsset::RTTI_Type());
            SetupThumbnails();
        }

        void ParticleMaterialThumbnailSystemComponent::Deactivate()
        {
            TeardownThumbnails();
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusDisconnect();
            AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        }

        void ParticleMaterialThumbnailSystemComponent::SetupThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;

            ThumbnailerRequestBus::Broadcast(&ThumbnailerRequests::RegisterThumbnailProvider, MAKE_TCACHE(Thumbnails::ParticleMaterialThumbnailCache));
        }

        void ParticleMaterialThumbnailSystemComponent::TeardownThumbnails()
        {
            using namespace AzToolsFramework::Thumbnailer;

            ThumbnailerRequestBus::Broadcast(
                &ThumbnailerRequests::UnregisterThumbnailProvider, Thumbnails::ParticleMaterialThumbnailCache::ProviderName);
        }

        void ParticleMaterialThumbnailSystemComponent::OnApplicationAboutToStop()
        {
            TeardownThumbnails();
        }

        bool ParticleMaterialThumbnailSystemComponent::Installed() const
        {
            return true;
        }

        void ParticleMaterialThumbnailSystemComponent::RenderThumbnail([[maybe_unused]] AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey,
            [[maybe_unused]] int thumbnailSize)
        {
            return;
        }
    } // namespace Thumbnails
} // namespace OpenParticleSystem
