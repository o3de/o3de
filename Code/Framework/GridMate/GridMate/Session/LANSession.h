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
#ifndef GM_LAN_SESSION_H
#define GM_LAN_SESSION_H

#include <GridMate/Session/Session.h>
#include <GridMate/Session/LANSessionServiceBus.h>
#include <GridMate/Session/LANSessionServiceTypes.h>
#include <GridMate/GridMateService.h>

namespace GridMate
{
    class LANSessionService
        : public SessionService
        , public LANSessionServiceBus::Handler
    {
    public:
        GM_CLASS_ALLOCATOR(LANSessionService);
        GRIDMATE_SERVICE_ID(LANSessionService);

        LANSessionService(const SessionServiceDesc& desc);
        ~LANSessionService() override;

        bool IsReady() const override { return m_gridMate != nullptr; }
        
        /////////////////////////
        // LANSessionServiceBus        
        GridSession* HostSession(const LANSessionParams& params, const CarrierDesc& carrierDesc) override;
        GridSession* JoinSessionBySearchInfo(const LANSearchInfo& searchInfo, const JoinParams& params, const CarrierDesc& carrierDesc) override;
        GridSession* JoinSessionBySessionIdInfo(const SessionIdInfo* info, const JoinParams& params, const CarrierDesc& carrierDesc) override;
        GridSearch* StartGridSearch(const LANSearchParams& params) override;        
        /////////////////////////

    protected:
        void OnServiceRegistered(IGridMate* gridMate) override;
        void OnServiceUnregistered(IGridMate* gridMate) override;
    };
}   // namespace GridMate

#endif  // GM_LAN_SESSION_H
