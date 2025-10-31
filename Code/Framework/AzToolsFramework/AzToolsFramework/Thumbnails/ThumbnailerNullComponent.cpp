/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerNullComponent.h>
#include <AzToolsFramework/Thumbnails/MissingThumbnail.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        ThumbnailerNullComponent::ThumbnailerNullComponent() :
            m_nullThumbnail()
        {
        }

        ThumbnailerNullComponent::~ThumbnailerNullComponent() = default;

        void ThumbnailerNullComponent::Activate()
        {
            BusConnect();
        }

        void ThumbnailerNullComponent::Deactivate()
        {
            BusDisconnect();
        }

        void ThumbnailerNullComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ThumbnailerNullComponent, AZ::Component>();
            }
        }

        void ThumbnailerNullComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("ThumbnailerService"));
        }

        void ThumbnailerNullComponent::RegisterThumbnailProvider(AzToolsFramework::Thumbnailer::SharedThumbnailProvider /*provider*/)
        {
        }

        void ThumbnailerNullComponent::UnregisterThumbnailProvider(const char* /*providerName*/)
        {
        }

        AzToolsFramework::Thumbnailer::SharedThumbnail ThumbnailerNullComponent::GetThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey /*key*/)
        {
            return m_nullThumbnail;
        }

        bool ThumbnailerNullComponent::IsLoading(AzToolsFramework::Thumbnailer::SharedThumbnailKey /*thumbnailKey*/)
        {
            return false;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
