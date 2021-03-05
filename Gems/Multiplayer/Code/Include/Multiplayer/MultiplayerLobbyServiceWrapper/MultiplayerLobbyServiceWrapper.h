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
#ifndef GEMS_MULTIPLAYER_MULTIPLAYERLOBBYSERVICEWRAPPER_MULTIPLAYERLOBBYSERVICEWRAPPER_H
#define GEMS_MULTIPLAYER_MULTIPLAYERLOBBYSERVICEWRAPPER_MULTIPLAYERLOBBYSERVICEWRAPPER_H

#include <AzCore/Memory/SystemAllocator.h>

#include <GridMate/GridMate.h>

#include "Multiplayer/MultiplayerLobbyBus.h"

namespace Multiplayer
{    
    class MultiplayerLobbyServiceWrapper
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiplayerLobbyServiceWrapper, AZ::SystemAllocator, 0);
        
    protected:
        MultiplayerLobbyServiceWrapper(const AZ::EntityId& multiplayerLobbyEntityId);
        
    public:
        
        virtual ~MultiplayerLobbyServiceWrapper();
        
        // MultiplayerLobbyServiceWrapper
        virtual const char* GetLobbyTitle() const = 0;

        virtual bool SanityCheck(GridMate::IGridMate* gridMate) = 0;
        virtual bool StartSessionService(GridMate::IGridMate* gridMate) = 0;
        virtual void StopSessionService(GridMate::IGridMate* gridMate) = 0;
        
        virtual GridMate::GridSession* CreateServer(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc);
        virtual GridMate::GridSearch* ListServers(GridMate::IGridMate* gridMate);
        virtual GridMate::GridSession* JoinSession(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo);       
        
    protected:
        const AZ::EntityId& GetTargetEntityId() const
        {
            return m_multiplayerLobbyEntityId;
        }
        
        virtual GridMate::GridSession* CreateServerForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc) = 0;
        virtual GridMate::GridSearch* ListServersForService(GridMate::IGridMate* gridMate) = 0;
        virtual GridMate::GridSession* JoinSessionForService(GridMate::IGridMate* gridMate, GridMate::CarrierDesc& carrierDesc, const GridMate::SearchInfo* searchInfo) = 0;
        
    private:
        AZ::EntityId m_multiplayerLobbyEntityId;        
    };
}

#endif
