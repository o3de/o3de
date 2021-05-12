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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerNullComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
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
            services.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void ThumbnailerNullComponent::RegisterContext(const char* /*contextName*/)
        {
        }

        void ThumbnailerNullComponent::UnregisterContext(const char* /*contextName*/)
        {
        }

        bool ThumbnailerNullComponent::HasContext(const char* /*contextName*/) const
        {
            return false;
        }

        void ThumbnailerNullComponent::RegisterThumbnailProvider(AzToolsFramework::Thumbnailer::SharedThumbnailProvider /*provider*/, const char* /*contextName*/)
        {
        }

        void ThumbnailerNullComponent::UnregisterThumbnailProvider(const char* /*providerName*/, const char* /*contextName*/)
        {
        }

        AzToolsFramework::Thumbnailer::SharedThumbnail ThumbnailerNullComponent::GetThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey /*key*/, const char* /*contextName*/)
        {
            return m_nullThumbnail;
        }

        bool ThumbnailerNullComponent::IsLoading(AzToolsFramework::Thumbnailer::SharedThumbnailKey /*thumbnailKey*/, const char* /*contextName*/)
        {
            return false;
        }
    } // namespace Thumbnailer
} // namespace AzToolsFramework
