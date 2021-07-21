/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

// ThumbnailerNullComponent is an alternative to ThumbnailerComponent that can be used by tools that don't use Qt.
// It doesn't do anything (hence "null"), but it allows the system and editor components that rely on the ThumbnailService
// to start up and function.

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ThumbnailContext;

        class ThumbnailerNullComponent
            : public AZ::Component
            , public AzToolsFramework::Thumbnailer::ThumbnailerRequestsBus::Handler
        {
        public:
            AZ_COMPONENT(ThumbnailerNullComponent, "{8009D651-3FAA-9815-B99E-AF174A3B29D4}")

            ThumbnailerNullComponent();
            virtual ~ThumbnailerNullComponent();

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            //////////////////////////////////////////////////////////////////////////
            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            //////////////////////////////////////////////////////////////////////////
            // ThumbnailerRequests
            //////////////////////////////////////////////////////////////////////////
            void RegisterContext(const char* contextName) override;
            void UnregisterContext(const char* contextName) override;
            bool HasContext(const char* contextName) const override;
            void RegisterThumbnailProvider(AzToolsFramework::Thumbnailer::SharedThumbnailProvider provider, const char* contextName) override;
            void UnregisterThumbnailProvider(const char* providerName, const char* contextName) override;
            AzToolsFramework::Thumbnailer::SharedThumbnail GetThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, const char* contextName) override;
            bool IsLoading(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, const char* contextName) override;
        private:
            AzToolsFramework::Thumbnailer::SharedThumbnail m_nullThumbnail;
        };
    } // Thumbnailer
} // namespace AzToolsFramework
