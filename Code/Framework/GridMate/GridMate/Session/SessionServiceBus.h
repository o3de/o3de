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