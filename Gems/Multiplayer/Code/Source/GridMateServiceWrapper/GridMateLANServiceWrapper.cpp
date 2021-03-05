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
#include "Multiplayer_precompiled.h"

#include <GridMate/Session/LANSession.h>

#include "Multiplayer/GridMateServiceWrapper/GridMateLANServiceWrapper.h"

#include "Multiplayer/MultiplayerUtils.h"
#include <Multiplayer_Traits_Platform.h>

namespace Multiplayer
{
    bool GridMateLANServiceWrapper::SanityCheck(GridMate::IGridMate* gridMate)
    {
        // Nothing in LAN Session Service we need to sanity check
        (void)gridMate;
        return true;
    }
    
    bool GridMateLANServiceWrapper::StartSessionService(GridMate::IGridMate* gridMate)
    {
        Multiplayer::LAN::StartSessionService(gridMate);
        return GridMate::HasGridMateService<GridMate::LANSessionService>(gridMate);
    }
    
    void GridMateLANServiceWrapper::StopSessionService(GridMate::IGridMate* gridMate)
    {
        Multiplayer::LAN::StopSessionService(gridMate);
    }    
    
    GridMate::GridSession* GridMateLANServiceWrapper::CreateServerForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMateServiceParams& params)
    {
        GridMate::GridSession* gridSession = nullptr;
        
        // Setup and create the LANSessionParams
        GridMate::LANSessionParams sessionParams;
        params.AssignSessionParams(sessionParams);
        sessionParams.m_port = GetServerPort(params);
        
        EBUS_EVENT_ID_RESULT(gridSession,gridMate,GridMate::LANSessionServiceBus,HostSession,sessionParams,carrierDesc);
        
        return gridSession;
    }
    
    GridMate::GridSearch* GridMateLANServiceWrapper::ListServersForService(GridMate::IGridMate* gridMate, const GridMateServiceParams& params)
    {
        GridMate::GridSearch* retVal = nullptr;
        
        GridMate::LANSearchParams searchParams;
        searchParams.m_serverPort = GetServerPort(params);
        searchParams.m_listenPort = 0;
        searchParams.m_version = params.m_version;
        searchParams.m_familyType = static_cast<GridMate::Driver::BSDSocketFamilyType>(params.FetchValueOrDefault<int>("gm_ipversion", GridMate::Driver::BSDSocketFamilyType::BSD_AF_INET));

#if AZ_TRAIT_MULTIPLAYER_ASSIGN_NETWORK_FAMILY
        AZ_Error(AZ_TRAIT_MULTIPLAYER_SESSION_NAME, searchParams.m_familyType == AZ_TRAIT_MULTIPLAYER_ADDRESS_TYPE, AZ_TRAIT_MULTIPLAYER_DRIVER_MESSAGE);
        searchParams.m_familyType = AZ_TRAIT_MULTIPLAYER_ADDRESS_TYPE;
#endif

        EBUS_EVENT_ID_RESULT(retVal, gridMate, GridMate::LANSessionServiceBus, StartGridSearch, searchParams);
        
        return retVal;
    }    
    
    GridMate::GridSession* GridMateLANServiceWrapper::JoinSessionForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo)
    {                
        GridMate::GridSession* gridSession = nullptr;
        const GridMate::LANSearchInfo& lanSearchInfo = static_cast<const GridMate::LANSearchInfo&>(*searchInfo);
        GridMate::JoinParams joinParams;
        
        EBUS_EVENT_ID_RESULT(gridSession, gridMate, GridMate::LANSessionServiceBus, JoinSessionBySearchInfo, lanSearchInfo, joinParams, carrierDesc);
        
        return gridSession;
    }
    
    int GridMateLANServiceWrapper::GetServerPort(const GridMateServiceParams& params) const
    {
        // GamePort is reserved for game traffic, we want to go 1 above it to manage our server duties. i.e. Responding to search requests.
        return params.FetchValueOrDefault<int>("cl_clientport", 0) + 1;
    }
}