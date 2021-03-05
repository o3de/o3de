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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <AzCore/EBus/EBus.h>

namespace GridMate
{
    class IGridMate;
    class GameLiftClientService;

    class GameLiftClientServiceEvents
        : public AZ::EBusTraits
    {
    public:
        typedef IGridMate* BusIdType; ///< Use the GridMate instance as an ID
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual ~GameLiftClientServiceEvents() {}

        /*!
        * Called when GameLift service is ready to use
        */
        virtual void OnGameLiftSessionServiceReady(GameLiftClientService*) {}

        /*!
        * Called when GameLift service is failed to initialize
        */
        virtual void OnGameLiftSessionServiceFailed(GameLiftClientService*, const AZStd::string&) {}
    };
    typedef AZ::EBus<GameLiftClientServiceEvents> GameLiftClientServiceEventsBus;
}

#endif
