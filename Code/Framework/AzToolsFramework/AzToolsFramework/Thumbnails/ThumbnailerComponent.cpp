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
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>

#include <QApplication>
#include <QStyle>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {

        ThumbnailerComponent::ThumbnailerComponent()
        {
        }

        ThumbnailerComponent::~ThumbnailerComponent() = default;

        void ThumbnailerComponent::Activate()
        {
            int thumbnailSize = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
            RegisterContext(ThumbnailContext::DefaultContext, thumbnailSize);
            BusConnect();
        }

        void ThumbnailerComponent::Deactivate()
        {
            BusDisconnect();
            m_thumbnails.clear();
        }

        void ThumbnailerComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ThumbnailerComponent, AZ::Component>();
            }
        }

        void ThumbnailerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void ThumbnailerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ThumbnailerService", 0x65422b97));
        }

        void ThumbnailerComponent::RegisterContext(const char* contextName, int thumbnailSize)
        {
            AZ_Assert(m_thumbnails.find(contextName) == m_thumbnails.end(), "Context %s already registered", contextName);
            m_thumbnails[contextName] = AZStd::make_shared<ThumbnailContext>(thumbnailSize);
        }

        void ThumbnailerComponent::UnregisterContext(const char* contextName)
        {
            AZ_Assert(m_thumbnails.find(contextName) != m_thumbnails.end(), "Context %s not registered", contextName);
            m_thumbnails.erase(contextName);
        }

        bool ThumbnailerComponent::HasContext(const char* contextName) const
        {
            return m_thumbnails.find(contextName) != m_thumbnails.end();
        }

        void ThumbnailerComponent::RegisterThumbnailProvider(SharedThumbnailProvider provider, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            it->second->RegisterThumbnailProvider(provider);
        }

        void ThumbnailerComponent::UnregisterThumbnailProvider(const char* providerName, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            it->second->UnregisterThumbnailProvider(providerName);
        }

        SharedThumbnail ThumbnailerComponent::GetThumbnail(SharedThumbnailKey key, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            return it->second->GetThumbnail(key);
        }

        bool ThumbnailerComponent::IsLoading(SharedThumbnailKey key, const char* contextName)
        {
            auto it = m_thumbnails.find(contextName);
            AZ_Assert(it != m_thumbnails.end(), "Context %s not registered", contextName);
            return it->second->IsLoading(key);
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

