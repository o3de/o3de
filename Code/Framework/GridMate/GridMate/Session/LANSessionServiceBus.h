/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SESSION_LANSESSIONBUS_H
#define GM_SESSION_LANSESSIONBUS_H

#include <GridMate/Session/SessionServiceBus.h>
#include <GridMate/Session/LANSessionServiceTypes.h>

namespace GridMate
{
    class LANSessionService;

    class LANSessionServiceInterface : public SessionServiceBusTraits
    {
    public:
        virtual GridSession* HostSession(const LANSessionParams& params, const CarrierDesc& carrierDesc) = 0;        
        virtual GridSession* JoinSessionBySearchInfo(const LANSearchInfo& searchInfo, const JoinParams& params, const CarrierDesc& carrierDesc) = 0;
        virtual GridSession* JoinSessionBySessionIdInfo(const SessionIdInfo* info, const JoinParams& params, const CarrierDesc& carrierDesc) = 0;
        
        virtual GridSearch* StartGridSearch(const LANSearchParams& params) = 0;
    };
    
    typedef AZ::EBus<LANSessionServiceInterface> LANSessionServiceBus;
}

#endif
