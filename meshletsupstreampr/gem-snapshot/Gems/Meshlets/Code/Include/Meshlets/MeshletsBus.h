
/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsRequests
        {
        public:
            AZ_RTTI(MeshletsRequests, "{c518217d-aa8a-47dc-8f2c-b63b4720d481}");
            virtual ~MeshletsRequests() = default;
            // Put your public methods here
        };

        class MeshletsBusTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
        };

        using MeshletsRequestBus = AZ::EBus<MeshletsRequests, MeshletsBusTraits>;
        using MeshletsInterface = AZ::Interface<MeshletsRequests>;

    } // namespace Meshlets
} // namespace AZ
