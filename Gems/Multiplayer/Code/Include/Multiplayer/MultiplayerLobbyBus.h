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

#ifndef GEMS_MULTIPLAYER_MULTIPLAYERLOBBYBUS_H
#define GEMS_MULTIPLAYER_MULTIPLAYERLOBBYBUS_H

#include <AzCore/Component/ComponentBus.h>

namespace GridMate
{
    struct SessionParams;
}

namespace Multiplayer
{
    class MultiplayerLobbyInterface
        : public AZ::ComponentBus
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        
        // MultiplayerLobbyInterface
        virtual int GetGamePort() const = 0;
        
        virtual void ShowError(const char* error) = 0;
        virtual void DismissError(bool force = false) = 0;
        
        virtual void ShowBusyScreen() = 0;
        virtual void DismissBusyScreen(bool force = false) = 0;
        
        virtual void ConfigureSessionParams(GridMate::SessionParams& sessionParams) = 0;        
    };
    
    using MultiplayerLobbyBus = AZ::EBus<MultiplayerLobbyInterface>;
}

#endif