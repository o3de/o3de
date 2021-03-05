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

#include "Multiplayer/MultiplayerEventsComponent.h"

#include "Multiplayer/BehaviorContext/GridSystemContext.h"

#include <AzCore/Serialization/SerializeContext.h>

#include <GridMate/NetworkGridMate.h>
#include <GridMate/NetworkGridMateSessionEvents.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Online/UserServiceTypes.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>
#include <Multiplayer_Traits_Platform.h>


namespace Multiplayer
{
    ///////////////////////////////////
    // SessionEventBusBehaviorHandler
    ///////////////////////////////////

    void SessionEventBusBehaviorHandler::OnSessionServiceReady()
    {
        Call(FN_OnSessionServiceReady);
    }
    void SessionEventBusBehaviorHandler::OnSessionCreated(GridMate::GridSession* gs)
    {
        Call(FN_OnSessionCreated, gs);
    }
    void SessionEventBusBehaviorHandler::OnSessionDelete(GridMate::GridSession* gs)
    {
        Call(FN_OnSessionDelete, gs);
    }
    void SessionEventBusBehaviorHandler::OnMemberJoined(GridMate::GridSession* gs, GridMate::GridMember* member)
    {
        Call(FN_OnMemberJoined, gs, member);
    }
    void SessionEventBusBehaviorHandler::OnMemberLeaving(GridMate::GridSession* gs, GridMate::GridMember* member)
    {
        Call(FN_OnMemberLeaving, gs, member);
    }
    void SessionEventBusBehaviorHandler::OnMemberKicked(GridMate::GridSession* gs, GridMate::GridMember* member, AZ::u8 kickreason)
    {
        Call(FN_OnMemberKicked, gs, member, kickreason);
    }
    void SessionEventBusBehaviorHandler::OnSessionJoined(GridMate::GridSession* gs)
    {
        Call(FN_OnSessionJoined, gs);
    }
    void SessionEventBusBehaviorHandler::OnSessionStart(GridMate::GridSession* gs)
    {
        Call(FN_OnSessionStart, gs);
    }
    void SessionEventBusBehaviorHandler::OnSessionEnd(GridMate::GridSession* gs)
    {
        Call(FN_OnSessionEnd, gs);
    }
    void SessionEventBusBehaviorHandler::OnSessionError(GridMate::GridSession* gs, const GridMate::string& msg)
    {
        Call(FN_OnSessionError, gs, msg);
    }

    ///////////////////////////////
    // MultiplayerEventsComponent
    ///////////////////////////////

    void MultiplayerEventsComponent::Init()
    {
    }

    void MultiplayerEventsComponent::Activate()
    {
    }
    void MultiplayerEventsComponent::Deactivate()
    {
    }

    /**
     * helper class to allow a constructor and destructor for MultiplayerEventsComponent
     */
    struct InternalMultiplayerEvents
        : public SessionEventBusBehaviorHandler
    {
        InternalMultiplayerEvents()
        {
            AZ_Assert(gEnv->pNetwork, "gEnv->pNetwork is nullptr");
            AZ_Assert(gEnv->pNetwork->GetGridMate(), "GridMate is nullptr");
            GridMate::SessionEventBus::Handler::BusConnect(gEnv->pNetwork->GetGridMate());
        }
        ~InternalMultiplayerEvents()
        {
            GridMate::SessionEventBus::Handler::BusDisconnect();
        }
        bool Connect(AZ::BehaviorValueParameter* id) override
        {
            AZ_UNUSED(id);

            AZ::BehaviorValueParameter thisGridMate(gEnv->pNetwork->GetGridMate());
            return AZ::Internal::EBusConnector<InternalMultiplayerEvents>::Connect(this, &thisGridMate);
        }
    };

    /**
    * helper functions to wrap a naked GridMate::PlayerId pointer
    * Note: it is valid for the GridMember to return a nullptr PlayerId for LAN connections
    */
    struct GridMatePlayerId
    {
        static GridMate::gridmate_string ToString(GridMate::PlayerId* playerId)
        {
            if (playerId)
            {
                return playerId->ToString();
            }
            static GridMate::gridmate_string s_blank("NOT_SUPPORTED");
            return s_blank;
        }

        static GridMate::ServiceType GetType(GridMate::PlayerId* playerId)
        {
            if (playerId)
            {
                return playerId->GetType();
            }
            return GridMate::ST_MAX;
        }
    };

    void MultiplayerEventsComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        GridMateSystemContext::Reflect(reflectContext);
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<MultiplayerEventsComponent, AZ::Component>()
                ->Version(1);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext);
        if (behaviorContext)
        {
            behaviorContext->EBus<GridMate::SessionEventBus>("MultiplayerEvents")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Handler<InternalMultiplayerEvents>()
                ;

            behaviorContext->Class<GridMate::GridSession>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Method("IsHost", &GridMate::GridSession::IsHost)
                ->Method("IsReady", &GridMate::GridSession::IsReady)
                ->Method("GetNumberOfMembers", &GridMate::GridSession::GetNumberOfMembers)
                ->Method("Leave", &GridMate::GridSession::Leave)
                ;

            behaviorContext->Class<GridMate::GridMember>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Method("GetName", &GridMate::GridMember::GetName)
                ->Method("IsHost", &GridMate::GridMember::IsHost)
                ->Method("IsLocal", &GridMate::GridMember::IsLocal)
                ->Method("IsInvited", &GridMate::GridMember::IsInvited)
                ->Method("IsReady", &GridMate::GridMember::IsReady)
                ->Method("IsTalking", &GridMate::GridMember::IsTalking)
                ->Method("GetPlayerId", &GridMate::GridMember::GetPlayerId)
                ;

            behaviorContext->Class<GridMate::PlayerId>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                ->Property("playerId", &GridMatePlayerId::ToString, nullptr)
                ->Property("type", &GridMatePlayerId::GetType, nullptr)
                ;

            // GridMate::ServiceType
            behaviorContext
                ->Enum<GridMate::ST_LAN>("ST_LAN")
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
            ->Enum<GridMate::ST_##CODENAME>("ST_"#CODENAME)

            AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
                ->Enum<GridMate::ST_STEAM>("ST_STEAM")
                ;

            behaviorContext->Class<AzFramework::NetQuery>("NetQuery")
                ->Method("IsEntityAuthoritative", &AzFramework::NetQuery::IsEntityAuthoritative)
                ;
        }
    }
}
