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

#include <AzCore/EBus/EBus.h>
#include "Session/GameLiftSessionDefs.h"

namespace GridMate
{
    class GameLiftClientService;
    struct GameLiftClientServiceDesc;
    class GameLiftServerService;
    struct GameLiftServerServiceDesc;
}

namespace GameLift
{
    class GameLiftRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual bool IsGameLiftServer() const = 0;

#if defined(BUILD_GAMELIFT_CLIENT)
        // Start GridMate client service for GameLift sessions
        virtual GridMate::GameLiftClientService* StartClientService(const GridMate::GameLiftClientServiceDesc& desc) = 0;

        // Stop GridMate client service for GameLift sessions
        virtual void StopClientService() = 0;

        // Returns current gamelift client service
        virtual GridMate::GameLiftClientService* GetClientService() = 0;
#endif

#if defined(BUILD_GAMELIFT_SERVER)
        // Start GridMate server service for GameLift sessions
        virtual GridMate::GameLiftServerService* StartServerService(const GridMate::GameLiftServerServiceDesc& desc) = 0;

        // Stop GridMate server service for GameLift sessions
        virtual void StopServerService() = 0;

        // Returns current gamelift server service
        virtual GridMate::GameLiftServerService* GetServerService() = 0;
#endif
    };
    using GameLiftRequestBus = AZ::EBus<GameLiftRequests>;
} // namespace GameLift
