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

#include <memory>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <ChatPlay/ChatPlayTypes.h>
#include <ChatPlay/ChatPlayBus.h>

#include "ChatPlay/ChatPlay.h"
#include "ChatPlay/ChatPlayCVars.h"
#include "Broadcast/BroadcastAPI.h"
#include <ChatPlay_Traits_Platform.h>

#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
#include "JoinIn/JoinInCVars.h"
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST

namespace ChatPlay
{

    class ChatPlaySystemComponent
        : public AZ::Component
        , protected ChatPlayRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(ChatPlaySystemComponent, "{20952273-903A-4B2F-9C64-EF75193B941A}");
        ChatPlaySystemComponent() = default;
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ChatPlayRequestBus interface implementation
        //
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        IBroadcast* GetBroadcastAPI() override { return m_broadcastAPI.get(); }
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST

        bool CreateChannel(const AZStd::string& channelId) override;
        void DestroyChannel(const AZStd::string& channelId) override;

        void DisconnectAll() override;

        void RegisterCredentials(const AZStd::string& username, const AZStd::string& oauthToken) override;
        void UnregisterCredentials(const AZStd::string& username) override;
        void UnregisterAllCredentials() override;

        void SendWhisperWithCallback(
            const AZStd::string& sender,
            const AZStd::string& recipient,
            const AZStd::string& message,
            const WhisperCallback& callback) override;

        WhisperToken SendWhisper(
            const AZStd::string& sender,
            const AZStd::string& recipient,
            const AZStd::string& message) override;

        bool CreateVote(const AZStd::string& voteId) override;
        void DestroyVote(const AZStd::string& voteId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SystemTickBus interface implementation
        void OnSystemTick() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ChatPlaySystemComponent(ChatPlaySystemComponent&) = delete;
        AZStd::shared_ptr<ChatPlay> m_chatPlay;
        AZStd::shared_ptr<ChatPlayCVars> m_chatPlayCVars;
        
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        AZStd::shared_ptr<JoinInCVars> m_joinInCVars;
        IBroadcastPtr m_broadcastAPI;
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST

        WhisperToken m_lastWhisperToken;

    };
}
