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
