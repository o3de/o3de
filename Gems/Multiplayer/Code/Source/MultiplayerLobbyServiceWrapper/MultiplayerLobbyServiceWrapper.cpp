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

#include "Multiplayer/MultiplayerLobbyServiceWrapper/MultiplayerLobbyServiceWrapper.h"

namespace Multiplayer
{
    MultiplayerLobbyServiceWrapper::MultiplayerLobbyServiceWrapper(const AZ::EntityId& multiplayerLobbyEntityId)
        : m_multiplayerLobbyEntityId(multiplayerLobbyEntityId)
    {    
    }
    
    MultiplayerLobbyServiceWrapper::~MultiplayerLobbyServiceWrapper()
    {
    }
    
    GridMate::GridSession* MultiplayerLobbyServiceWrapper::CreateServer(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc)
    {
        GridMate::GridSession* gridSession = nullptr;
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSession = CreateServerForService(gridMate,carrierDesc);
        }
        
        return gridSession;
    }
    
    GridMate::GridSearch* MultiplayerLobbyServiceWrapper::ListServers(GridMate::IGridMate* gridMate)
    {
        GridMate::GridSearch* gridSearch = nullptr;
        
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSearch = ListServersForService(gridMate);
        }
        
        return gridSearch;
    }
    
    GridMate::GridSession* MultiplayerLobbyServiceWrapper::JoinSession(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo)
    {
        GridMate::GridSession* gridSession = nullptr;
        
        if (StartSessionService(gridMate) && SanityCheck(gridMate))
        {
            gridSession = JoinSessionForService(gridMate, carrierDesc, searchInfo);
        }
        
        return gridSession;
    }
}