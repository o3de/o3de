/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            RegisterContext(ThumbnailContext::DefaultContext);
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

        void ThumbnailerComponent::RegisterContext(const char* contextName)
        {
            AZ_Assert(m_thumbnails.find(contextName) == m_thumbnails.end(), "Context %s already registered", contextName);
            m_thumbnails[contextName] = AZStd::make_shared<ThumbnailContext>();
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

