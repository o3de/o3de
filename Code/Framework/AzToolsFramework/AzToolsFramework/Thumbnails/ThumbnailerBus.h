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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

class QPixmap;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! Interaction with thumbnailer
        class ThumbnailerRequests
            : public AZ::EBusTraits
        {
        public:
            //! Add thumbnail context
            virtual void RegisterContext(const char* contextName, int thumbnailSize) = 0;

            //! Remove thumbnail context and all associated ThumbnailProviders
            virtual void UnregisterContext(const char* contextName) = 0;

            //! Return whether a given ThumbnailContext has been registered
            virtual bool HasContext(const char* contextName) const = 0;

            //! Add new thumbnail provider to ThumbnailContext
            virtual void RegisterThumbnailProvider(SharedThumbnailProvider provider, const char* contextName) = 0;

            //! Remove thumbnail provider from ThumbnailContext
            virtual void UnregisterThumbnailProvider(const char* providerName, const char* contextName) = 0;

            //! Retrieve thumbnail by key,
            //! if no thumbnail matching found, one of ThumbnailProviders will attempt to create
            //! If no compatible providers found, MissingThumbnail will be returned
            virtual SharedThumbnail GetThumbnail(SharedThumbnailKey thumbnailKey, const char* contextName) = 0;

            //! Return whether the thumbnail is loading.
           virtual bool IsLoading(SharedThumbnailKey thumbnailKey, const char* contextName) = 0;
        };

        using ThumbnailerRequestBus = AZ::EBus<ThumbnailerRequests>;
        using ThumbnailerRequestsBus = AZ::EBus<ThumbnailerRequests>; //deprecated

        //! Request product thumbnail to be rendered
        class ThumbnailerRendererRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZ::Data::AssetType BusIdType;

            static const bool EnableEventQueue = true;
            typedef AZStd::recursive_mutex MutexType;

            virtual void RenderThumbnail(SharedThumbnailKey thumbnailKey, int thumbnailSize) = 0;

            virtual bool Installed() const { return false; }
        };

        using ThumbnailerRendererRequestBus = AZ::EBus<ThumbnailerRendererRequests>;
        using ThumbnailerRendererRequestsBus = AZ::EBus<ThumbnailerRendererRequests>; //deprecated

        //! Notify that product thumbnail was rendered
        class ThumbnailerRendererNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef SharedThumbnailKey BusIdType;

            //! notify product thumbnail that the data is ready
            virtual void ThumbnailRendered(QPixmap& thumbnailImage) = 0;
            //! notify product thumbnail that the thumbnail failed to render
            virtual void ThumbnailFailedToRender() = 0;
        };

        using ThumbnailerRendererNotificationBus = AZ::EBus<ThumbnailerRendererNotifications>;
    } // namespace Thumbnailer
} // namespace AzToolsFramework
