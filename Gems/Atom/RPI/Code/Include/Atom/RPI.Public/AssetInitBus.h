/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace RPI
    {
        //! Bus for post-load initialization of assets.
        //! Assets that need to do post-load initialization should connect to this bus in their asset handler's LoadAssetData() function.
        //! Be sure to disconnect from this bus as soon as initialization is complete, as it will be called every frame.
        //! (Note this bus is needed rather than utilizing TickBus because TickBus is not protected by a mutex which means it can't be
        //! connected on an asset load job thread).
        class AssetInitEvents
            : public EBusTraits
        {
        public:
            // EBus Configuration
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            typedef AZStd::recursive_mutex  MutexType;

            //! This function is called every frame on the main thread to perform any necessary post-load initialization.
            //! Connect to the bus after loading the asset data, and disconnect when initialization is complete.
            //! @return whether initialization was successful
            virtual bool PostLoadInit() = 0;
        };

        using AssetInitBus = AZ::EBus<AssetInitEvents>;
    } // namespace RPI
} // namespace AZ
