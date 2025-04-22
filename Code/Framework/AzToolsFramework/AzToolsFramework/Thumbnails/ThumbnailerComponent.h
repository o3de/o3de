/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/set.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QObject>
#endif

class QString;
class QPixmap;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        class ThumbnailerComponent
            : public AZ::Component
            , public ThumbnailerRequestBus::Handler
        {
        public:
            AZ_COMPONENT(ThumbnailerComponent, "{80090CA5-6A3A-4554-B5FE-A6D74ECB2D84}")

            ThumbnailerComponent();
            virtual ~ThumbnailerComponent();

            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            // ThumbnailerRequestBus::Handler interface overrides...
            void RegisterThumbnailProvider(SharedThumbnailProvider provider) override;
            void UnregisterThumbnailProvider(const char* providerName) override;
            SharedThumbnail GetThumbnail(SharedThumbnailKey thumbnailKey) override;
            bool IsLoading(SharedThumbnailKey thumbnailKey) override;

        private:
            void Cleanup();

            struct ProviderCompare
            {
                bool operator()(const SharedThumbnailProvider& lhs, const SharedThumbnailProvider& rhs) const
                {
                    // sorting in reverse, higher priority means the provider should be considered first
                    return lhs->GetPriority() > rhs->GetPriority();
                }
            };

            //! Collection of thumbnail caches provided by this context
            AZStd::multiset<SharedThumbnailProvider, ProviderCompare> m_providers;
            //! Default missing thumbnail used when no thumbnail for given key can be found within this context
            SharedThumbnail m_missingThumbnail;
            //! Default loading thumbnail used when thumbnail is found by is not yet generated
            SharedThumbnail m_loadingThumbnail;
            //! Using placeholder object rather than inheritance for connecting signals and slots
            AZStd::unique_ptr<QObject> m_placeholderObject;
            //! Current number of jobs running.
            AZStd::set<SharedThumbnail> m_thumbnailsBeingLoaded;
        };
    } // Thumbnailer
} // namespace AssetBrowser
