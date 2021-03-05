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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <GridMate/NetworkGridMateSessionEvents.h>

namespace AZ
{
    class ReflectContext;
};

namespace GridMate
{
    class GridSession;
    class GridMember;
};

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(GridMate::IGridMate, "{36719AC6-5034-4C59-B27C-D1CE9E84EC11}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::GridSession, "{E7DAD8D4-0A6F-43F0-B9D7-943F28BCA3F6}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::GridMember, "{6776F991-3C8A-439B-81A5-0C8FEE4A58AC}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::string, "{9ED5D0E0-E278-4471-BD68-890F1141D4B5}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::gridmate_string, "{27C4D9CF-EEB5-4204-9EAE-4CF03E8CC636}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::PlayerId, "{1D45BA71-220A-48A3-B8BF-BC7949DFA7D0}");
};

namespace Multiplayer
{
    /**
    * Pass thru behavior for GridMate::SessionEventBus::Handler
    */
    class SessionEventBusBehaviorHandler
        : public GridMate::SessionEventBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(SessionEventBusBehaviorHandler, "{31E5EA10-42F6-4152-A924-7F6DAE1588BC}", AZ::SystemAllocator
            , OnSessionServiceReady
            , OnSessionCreated
            , OnSessionDelete
            , OnMemberJoined
            , OnMemberLeaving
            , OnMemberKicked
            , OnSessionJoined
            , OnSessionError
            , OnSessionStart
            , OnSessionEnd
        );

        // GridMate::SessionEventBus
        void OnSessionServiceReady() override;
        void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member) override;
        void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override;
        void OnMemberKicked(GridMate::GridSession* session, GridMate::GridMember* member, AZ::u8 kickreason) override;
        void OnSessionCreated(GridMate::GridSession* session) override;
        void OnSessionJoined(GridMate::GridSession* session) override;
        void OnSessionDelete(GridMate::GridSession* session) override;
        void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg) override;
        void OnSessionStart(GridMate::GridSession* session) override;
        void OnSessionEnd(GridMate::GridSession* session) override;
    };

    class MultiplayerEventsComponent
        : public AZ::Component
    {
    public:

        AZ_COMPONENT(MultiplayerEventsComponent,"{5969EE34-A090-4367-A6B5-464D1D5F3BF4}");

        // for behavior reflection
        static void Reflect(AZ::ReflectContext* reflectContext);

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;


    };
}