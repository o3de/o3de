/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

class QPixmap;
class QThreadPool;

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //! Interaction with thumbnailer
        class ThumbnailerRequests
            : public AZ::EBusTraits
        {
        public:
            //! Add new thumbnail provider
            virtual void RegisterThumbnailProvider(SharedThumbnailProvider provider) = 0;

            //! Remove thumbnail provider
            virtual void UnregisterThumbnailProvider(const char* providerName) = 0;

            //! Retrieve thumbnail by key,
            //! if no thumbnail matching found, one of ThumbnailProviders will attempt to create
            //! If no compatible providers found, MissingThumbnail will be returned
            virtual SharedThumbnail GetThumbnail(SharedThumbnailKey thumbnailKey) = 0;

            //! Return whether the thumbnail is loading.
            virtual bool IsLoading(SharedThumbnailKey thumbnailKey) = 0;
        };

        using ThumbnailerRequestBus = AZ::EBus<ThumbnailerRequests>;

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

        //! Notify that product thumbnail was rendered
        class ThumbnailerRendererNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef SharedThumbnailKey BusIdType;

            //! notify product thumbnail that the data is ready
            virtual void ThumbnailRendered(const QPixmap& thumbnailImage) = 0;
            //! notify product thumbnail that the thumbnail failed to render
            virtual void ThumbnailFailedToRender() = 0;
        };

        using ThumbnailerRendererNotificationBus = AZ::EBus<ThumbnailerRendererNotifications>;
    } // namespace Thumbnailer
} // namespace AzToolsFramework
