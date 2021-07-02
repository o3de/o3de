/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ThumbnailContext;

        class ThumbnailerComponent
            : public AZ::Component
            , public ThumbnailerRequestsBus::Handler
        {
        public:
            AZ_COMPONENT(ThumbnailerComponent, "{80090CA5-6A3A-4554-B5FE-A6D74ECB2D84}")

            ThumbnailerComponent();
            virtual ~ThumbnailerComponent();

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            //////////////////////////////////////////////////////////////////////////
            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            //////////////////////////////////////////////////////////////////////////
            // ThumbnailerRequests
            //////////////////////////////////////////////////////////////////////////
            void RegisterContext(const char* contextName) override;
            void UnregisterContext(const char* contextName) override;
            bool HasContext(const char* contextName) const override;
            void RegisterThumbnailProvider(SharedThumbnailProvider provider, const char* contextName) override;
            void UnregisterThumbnailProvider(const char* providerName, const char* contextName) override;
            SharedThumbnail GetThumbnail(SharedThumbnailKey thumbnailKey, const char* contextName) override;
            bool IsLoading(SharedThumbnailKey thumbnailKey, const char* contextName) override;

        private:
            AZStd::unordered_map<AZStd::string, AZStd::shared_ptr<ThumbnailContext>> m_thumbnails;
        };
    } // Thumbnailer
} // namespace AssetBrowser
