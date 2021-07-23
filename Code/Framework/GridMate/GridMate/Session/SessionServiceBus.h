/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SESSION_SESSIONSERVICEBUS_H
#define GM_SESSION_SESSIONSERVICEBUS_H

#include <AzCore/EBus/EBus.h>
#include <GridMate/Session/Session.h>

namespace GridMate
{
    class IGridMate;
    
    class SessionServiceBusTraits : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single; ///< Only one service may provide this interface
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef IGridMate* BusIdType; ///< Use the GridMate instance as an ID
        //////////////////////////////////////////////////////////////////////////
    };
}

#endif
