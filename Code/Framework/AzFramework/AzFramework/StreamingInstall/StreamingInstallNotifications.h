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
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    namespace StreamingInstall
    {
        ////////////////////////////////////////////////////////////////////////////////////////
        class StreamingInstallChunkNotifications : public AZ::EBusTraits
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////////
            //! EBus Trait: StreaimingInstall chunk notifications can be handled by multiple listeners
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! EBus Trait: StreamingInstall chunk notifications are addressed to multiple addresses
            //! Events that are addressed to an ID are received by handlers that are connected to that ID
            typedef AZStd::string BusIdType;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override to be notified when a chunk has completed downloading. This is a responce from
            //! the RegisterChunkInstalledCallbacks event emitted from the platform implementation
            //! \param[in] the ID for the chunk that has downloaded
            virtual void OnChunkDownloadComplete(const AZStd::string& chunkId) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override to be notified when a chunk's download progress has changed. This is a responce
            //! from the StreamingInstallRequests::BroadcastChunkProgress event
            //! \param[in] the ID for the monitored chunk, the current download progress as a 0.0 to 1.0 value
            virtual void OnChunkProgressChanged(const AZStd::string& chunkId, float progress) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override to be notified if a chunk has been installed. This is a responce
            //! from the StreamingInstallRequests::IsChunkInstalled event
            //! \param[in] the ID of the chunk to query, installed flag for the chunk (true if installed and false otherwise).
            virtual void OnQueryChunkInstalled(const AZStd::string& chunkId, bool installed) = 0;

        };

        using StreamingInstallChunkNotificationBus = AZ::EBus<StreamingInstallChunkNotifications>;

        ////////////////////////////////////////////////////////////////////////////////////////
        class StreamingInstallPackageNotifications : public AZ::EBusTraits
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////////
            //! EBus Trait: StreaimingInstall package notifications can be handled by multiple listeners
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! EBus Trait: StreamingInstall package notifications are addressed to a single address
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override to be notified the packages download progress has changed. This is a response
            //! from the StreamingInstallRequests::BroadcastOverallProgress event
            //! \param[in] the current download progress for the entire package as a 0.0 to 1.0 value
            virtual void OnPackageProgressChanged(float progress) = 0;
        };

        using StreamingInstallPackageNotificationBus = AZ::EBus<StreamingInstallPackageNotifications>;
    } //namespace StreamingInstall
}
