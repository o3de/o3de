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

#include <AzCore/RTTI/BehaviorContext.h>
#include <GridMate/NetworkGridMateSessionEvents.h>
#include <GridMate/Types.h>

#include "Multiplayer/MultiplayerEventsComponent.h"

namespace GridMate
{
    class GridSession;
    class GridSearch;
    struct SearchInfo;
    struct CarrierDesc;
};

namespace Multiplayer
{
    class GridMateServiceWrapper;
    struct GridMateServiceParams;
};

namespace Multiplayer
{
    /**
     * Allows behavior contexts to describe network session parameters
     */
    struct SessionDesc
    {
    public:
        AZ_TYPE_INFO(SessionDesc, "{AC88F475-C5E1-4FC9-ADFB-D4C595E05CD6}");

        SessionDesc()
            : m_gamePort(34003)
            , m_maxPlayerSlots(16)
            , m_enableDisconnectDetection(true)
            , m_connectionTimeoutMS(500)
            , m_threadUpdateTimeMS(30)
            , m_mapName()
            , m_serverName()
            , m_serviceType(GridMate::ST_MAX)

        {
        }

        AZ::u16 m_gamePort;
        AZ::u16 m_maxPlayerSlots;
        bool m_enableDisconnectDetection;
        AZ::u32 m_connectionTimeoutMS;
        AZ::s32 m_threadUpdateTimeMS;
        AZStd::string m_mapName;
        AZStd::string m_serverName;
        GridMate::ServiceType m_serviceType;
    };

    /**
     * Exposes network session management from behavior contexts
     */
    class SessionManagerInterface
        : public AZ::ComponentBus
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        // Signal events
        virtual bool StartHost(const SessionDesc& sessionDesc) = 0;
        virtual bool Close() = 0;

        // Sink events
        virtual void OnHostSessionStarted(GridMate::GridSession* session) = 0;
    };
    using SessionManagerBus = AZ::EBus<SessionManagerInterface>;

    /**
    * The GridMate system component methods
    */
    namespace GridMateSystemContext
    {
        /**
        * registers the GridMate system for behavior contexts
        */
        void Reflect(AZ::ReflectContext* reflectContext);

        /**
        * helper method for fetch parameters from a session description or the console where the 'param' is filled out when this returns true
        */
        bool FetchParam(const char* key, SessionDesc sessionDesc, GridMate::GridSessionParam& param);

        /**
        * helper method to fill out a CarrierDesc for behavior contexts
        */
        void InitCarrierDesc(const GridMateServiceParams& gridMateServiceParams, GridMate::CarrierDesc& carrierDesc);

        /**
        * Creates a MultiplayerLobbyServiceWrapper based on the GridMate::ServiceType for an entity
        */
        GridMateServiceWrapper* RegisterServiceWrapper(GridMate::ServiceType gridServiceType);
    };
}

