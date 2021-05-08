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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace LUAEditor
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
} // namespace LUAEditor
