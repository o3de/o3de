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
