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
#include "ChatPlay_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "ChatPlaySystemComponent.h"

// Helper macro for reflecting class enums
#define CHATPLAY_BC_ENUM(className, enumName) Enum<(int)className::enumName>(#enumName)
// Helper macro for reflecting events
#define CHATPLAY_BC_EVENT(className, eventName) Event(#eventName, &className::Events::eventName)

namespace ChatPlay
{
    class ChatPlayNotificationBusHandler
        : public ChatPlayNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ChatPlayNotificationBusHandler, "{6AB7C392-9936-407F-8530-45387FA80059}", AZ::SystemAllocator,
            OnWhisperSent);

        void OnWhisperSent(WhisperResult result) override
        {
            Call(FN_OnWhisperSent, result);
        }
    };

    class ChatPlayChannelNotificationBusHandler
        : public ChatPlayChannelNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ChatPlayChannelNotificationBusHandler, "{9416741E-DC90-4366-89A8-7851909E0869}", AZ::SystemAllocator,
            OnConnectionStateChanged, OnKeywordMatched);

        void OnConnectionStateChanged(ConnectionState state) override
        {
            Call(FN_OnConnectionStateChanged, state);
        }

        void OnKeywordMatched(const AZStd::string& keyword, const AZStd::string& match, const AZStd::string& username) override
        {
            Call(FN_OnKeywordMatched, keyword, match, username);
        }
    };

    void ChatPlaySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ChatPlaySystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ChatPlaySystemComponent>("ChatPlaySystemComponent", "System Component necessary for using Twitch ChatPlay features")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            // Reflecting our enums as part of a class makes them more intuitive to use from Lua.
            // E.g. from Lua "ConnectionState.Connected" closely matches the C++ "ConnectionState::Connected".

            behaviorContext->Class<ConnectionState>("ConnectionState")
                ->CHATPLAY_BC_ENUM(ConnectionState, Disconnected)
                ->CHATPLAY_BC_ENUM(ConnectionState, Connecting)
                ->CHATPLAY_BC_ENUM(ConnectionState, Connected)
                ->CHATPLAY_BC_ENUM(ConnectionState, Error)
                ->CHATPLAY_BC_ENUM(ConnectionState, Failed)
                ;

            behaviorContext->Class<WhisperResult>("WhisperResult")
                ->CHATPLAY_BC_ENUM(WhisperResult, Success)
                ->CHATPLAY_BC_ENUM(WhisperResult, MissingOAuthToken)
                ->CHATPLAY_BC_ENUM(WhisperResult, ConnectionError)
                ->CHATPLAY_BC_ENUM(WhisperResult, AuthenticationError)
                ;

            behaviorContext->EBus<ChatPlayRequestBus>("ChatPlayRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, CreateChannel)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, DestroyChannel)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, DisconnectAll)

                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, RegisterCredentials)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, UnregisterCredentials)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, UnregisterAllCredentials)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, SendWhisper)

                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, CreateVote)
                ->CHATPLAY_BC_EVENT(ChatPlayRequestBus, DestroyVote)
                ;

            behaviorContext->EBus<ChatPlayNotificationBus>("ChatPlayNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<ChatPlayNotificationBusHandler>()
                ;

            behaviorContext->EBus<ChatPlayChannelRequestBus>("ChatPlayChannelRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->CHATPLAY_BC_EVENT(ChatPlayChannelRequestBus, Connect)
                ->CHATPLAY_BC_EVENT(ChatPlayChannelRequestBus, Disconnect)

                ->CHATPLAY_BC_EVENT(ChatPlayChannelRequestBus, GetConnectionState)
                ;

            behaviorContext->EBus<ChatPlayChannelNotificationBus>("ChatPlayChannelNotificationBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Handler<ChatPlayChannelNotificationBusHandler>()
                ;

            behaviorContext->EBus<ChatPlayVoteRequestBus>("ChatPlayVoteRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, AddOption)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, RemoveOption)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, ConfigureOption)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, OptionExists)

                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, GetOptionCount)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetOptionCount)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, GetOptionEnabled)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetOptionEnabled)

                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetChannel)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, ClearChannel)

                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetEnableStateAll)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetCountAll)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, SetVoterLimiting)
                ->CHATPLAY_BC_EVENT(ChatPlayVoteRequestBus, ResetVotedList)
                ;
        }
    }

    void ChatPlaySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ChatPlayService"));
    }

    void ChatPlaySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ChatPlayService"));
    }

    void ChatPlaySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ChatPlaySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    bool ChatPlaySystemComponent::CreateChannel(const AZStd::string& channelId)
    {
        if (m_chatPlay && m_chatPlay->GetChatChannel(channelId).lock())
        {
            return true;
        }

        return false;
    }

    void ChatPlaySystemComponent::DestroyChannel(const AZStd::string& channelId)
    {
        if (m_chatPlay)
        {
            m_chatPlay->DestroyChatChannel(channelId);
        }
    }

    void ChatPlaySystemComponent::DisconnectAll()
    {
        if (m_chatPlay)
        {
            m_chatPlay->DisconnectAll();
        }
    }

    void ChatPlaySystemComponent::RegisterCredentials(const AZStd::string& username, const AZStd::string& oauthToken)
    {
        m_chatPlay->RegisterCredentials(username, oauthToken);
    }

    void ChatPlaySystemComponent::UnregisterCredentials(const AZStd::string& username)
    {
        m_chatPlay->UnregisterCredentials(username);
    }

    void ChatPlaySystemComponent::UnregisterAllCredentials()
    {
        m_chatPlay->UnregisterAllCredentials();
    }

    void ChatPlaySystemComponent::SendWhisperWithCallback(
        const AZStd::string& sender,
        const AZStd::string& recipient,
        const AZStd::string& message,
        const WhisperCallback& callback)
    {
        m_chatPlay->SendWhisper(sender, recipient, message, callback);
    }

    WhisperToken ChatPlaySystemComponent::SendWhisper(
        const AZStd::string& sender,
        const AZStd::string& recipient,
        const AZStd::string& message)
    {
        WhisperToken token = ++m_lastWhisperToken;

        WhisperCallback cb = [](WhisperResult result) {
            ChatPlayNotificationBus::Broadcast(&ChatPlayNotificationBus::Events::OnWhisperSent, result);
        };

        m_chatPlay->SendWhisper(sender, recipient, message, cb);

        return token;
    }

    bool ChatPlaySystemComponent::CreateVote(const AZStd::string& voteId)
    {
        if (m_chatPlay && m_chatPlay->GetVoteManager()->GetVote(voteId).lock())
        {
            return true;
        }

        return false;
    }

    void ChatPlaySystemComponent::DestroyVote(const AZStd::string& voteId)
    {
        if (m_chatPlay)
        {
            m_chatPlay->GetVoteManager()->DestroyVote(voteId);
        }
    }

    void ChatPlaySystemComponent::Init()
    {
    }

    void ChatPlaySystemComponent::Activate()
    {
        m_chatPlayCVars = ChatPlayCVars::GetInstance();
#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        m_joinInCVars = JoinInCVars::GetInstance();
        m_chatPlay = ChatPlay::CreateInstance();
        m_broadcastAPI = CreateBroadcastAPI();
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST

        ChatPlayRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void ChatPlaySystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        ChatPlayRequestBus::Handler::BusDisconnect();

#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        m_chatPlay.reset();
        m_broadcastAPI.reset();
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        m_chatPlayCVars.reset();

    }

    void ChatPlaySystemComponent::OnSystemTick()
    {
        if (m_chatPlay) // Only dispatch events if ChatPlay is available and was enabled
        {
            m_chatPlay->DispatchEvents();
        }

#if AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST
        if (m_broadcastAPI)
        {
            m_broadcastAPI->DispatchEvents();
        }
#endif // AZ_TRAIT_CHATPLAY_JOIN_AND_BROADCAST

    }
}

#undef CHATPLAY_BC_ENUM
#undef CHATPLAY_BC_EVENT
