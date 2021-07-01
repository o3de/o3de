/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
