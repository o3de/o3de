/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AzFramework
{
    namespace StreamingInstall
    {
        ////////////////////////////////////////////////////////////////////////////////////////
        // EBUS interface used for setting monitoring and qeurying package and chunk install progress
        // and requesting a change to the chunk install order.
        class StreamingInstallRequests : public AZ::EBusTraits
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////////
            //! EBus Trait: requests can only be sent to and addressed by a single instance.
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override will use platform API to request reports on package install progress changes.
            virtual void BroadcastOverallProgress() = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override will use platform API to request reports on chunk install progress changes.
            //! \param[in] the chunk ID to monitor
            virtual void BroadcastChunkProgress(const char* chunkId) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override will use platform API to request a change to chunk install order. Chunks will
            //! install in the order they appear in the vector from front to back
            //! \param[in] a vector of chunk IDs in the desired new install order.
            virtual void ChangeChunkPriority(const AZStd::vector<const char*>& chunkIds) = 0;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Override will use platform API to query if a chunk is installed. 
            //! \param[in] Id of the chunk to query. 
            virtual void IsChunkInstalled(const char* chunkId) = 0;
        };

        using StreamingInstallRequestBus = AZ::EBus<StreamingInstallRequests>;
    } // namespace StreamingInstall
}
