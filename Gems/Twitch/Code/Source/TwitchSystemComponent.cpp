/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/time.h>
#include "TwitchSystemComponent.h"
#include "TwitchReflection.h"
#include <Twitch_Traits_Platform.h>
namespace Twitch
{
    TwitchSystemComponent::TwitchSystemComponent() 
        : m_receiptCounter(0)
    {
    }

    void TwitchSystemComponent::OnSystemTick()
    {
        if( m_twitchREST != nullptr)
            m_twitchREST->FlushEvents();
    }
    
    void TwitchSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TwitchSystemComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<TwitchSystemComponent>("Twitch", "Provides access to Twitch \"Friends\", \"Rich Presence\" APIs")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context)) 
        {
            Internal::Reflect(*behaviorContext);
        }
    }

    void TwitchSystemComponent::SetApplicationID(const AZStd::string& twitchApplicationID)
    {
        /*
        ** THIS CAN ONLY BE SET ONCE!!!!!!
        */

        if (m_applicationID.empty())
        {
            if (IsValidTwitchAppID(twitchApplicationID))
            {
                m_applicationID = twitchApplicationID;
            }
            else
            {
                AZ_Warning("TwitchSystemComponent::SetApplicationID", false, "Invalid Twitch Application ID! Request ignored!");
            }
        }
        else
        {
            AZ_Warning("TwitchSystemComponent::SetApplicationID", false, "Twitch Application ID is already set! Request ignored!");
        }
    }

    AZStd::string TwitchSystemComponent::GetApplicationID() const
    {
        return m_applicationID;
    }

    AZStd::string TwitchSystemComponent::GetSessionID() const
    {
        return m_cachedSessionID;
    }

    AZStd::string TwitchSystemComponent::GetUserID() const
    {
        return m_cachedClientID;
    }

    AZStd::string TwitchSystemComponent::GetOAuthToken() const
    {
        return m_cachedOAuthToken;
    }

    void TwitchSystemComponent::SetUserID(ReceiptID& receipt, const AZStd::string& userId)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        ResultCode rc(ResultCode::InvalidParam);
        if (IsValidFriendID(userId))
        {
            m_cachedClientID = userId;
            rc = ResultCode::Success;
        }

        TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UserIDNotify, StringValue(userId, receipt, rc));
    }

    void TwitchSystemComponent::SetOAuthToken(ReceiptID& receipt, const AZStd::string& token)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        ResultCode rc(ResultCode::InvalidParam);
        if (IsValidOAuthToken(token))
        {
            m_cachedOAuthToken = token;
            rc = ResultCode::Success;
        }

        TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::OAuthTokenNotify, StringValue(token, receipt, rc));
    }

    void TwitchSystemComponent::RequestUserID(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UserIDNotify, StringValue(m_cachedClientID, receipt, ResultCode::Success));
    }

    void TwitchSystemComponent::RequestOAuthToken(ReceiptID& receipt)
    {
        // always return a receipt.
        receipt.SetID(GetReceipt());

        TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::OAuthTokenNotify, StringValue(m_cachedOAuthToken, receipt, ResultCode::Success));
    }

    void TwitchSystemComponent::GetUser(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetUser, UserInfoValue(UserInfo(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->GetUser(receipt);
        }
    }

    void TwitchSystemComponent::ResetFriendsNotificationCount(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::ResetFriendsNotificationCountNotify, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->ResetFriendsNotificationCount(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendNotificationCount(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendNotificationCount, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendNotificationCount(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendRecommendations(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendRecommendations, FriendRecommendationValue(FriendRecommendationList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendRecommendations(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriends(ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriends, GetFriendValue(GetFriendReturn(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriends(receipt, friendID, cursor);
        }
    }

    void TwitchSystemComponent::GetFriendStatus(ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if ((!sourceFriendID.empty() && !IsValidFriendID(sourceFriendID)) || !IsValidFriendID(targetFriendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if(rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendStatus, FriendStatusValue(FriendStatus(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetFriendStatus(receipt, sourceFriendID, targetFriendID);
        }
    }

    void TwitchSystemComponent::AcceptFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::AcceptFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->AcceptFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::GetFriendRequests(ReceiptID& receipt, const AZStd::string& cursor)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendRequests, FriendRequestValue(FriendRequestResult(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->GetFriendRequests(receipt, cursor);
        }
     }

    void TwitchSystemComponent::CreateFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::CreateFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->CreateFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::DeclineFriendRequest(ReceiptID& receipt, const AZStd::string& friendID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidFriendID(friendID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::DeclineFriendRequest, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->DeclineFriendRequest(receipt, friendID);
        }
    }

    void TwitchSystemComponent::UpdatePresenceStatus(ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext)
    {
        receipt.SetID(GetReceipt());

        ResultCode rc(ResultCode::Success);

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if ((activityType == PresenceActivityType::Playing) && !m_twitchREST->IsValidGameContext(gameContext) )
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdatePresenceStatus, Int64Value(0, receipt, rc));
        }
        else
        {
            m_twitchREST->UpdatePresenceStatus(receipt, availability, activityType, gameContext);
        }
    }
    
    void TwitchSystemComponent::GetPresenceStatusofFriends(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetPresenceStatusofFriends, PresenceStatusValue(PresenceStatusList(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->GetPresenceStatusofFriends(receipt);
        }
    }
    
    void TwitchSystemComponent::GetPresenceSettings(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetPresenceSettings, PresenceSettingsValue(PresenceSettings(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->GetPresenceSettings(receipt);
        }
    }

    void TwitchSystemComponent::UpdatePresenceSettings(ReceiptID& receipt, bool isInvisible, bool shareActivity)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedClientID.empty() || m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdatePresenceSettings, PresenceSettingsValue(PresenceSettings(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->UpdatePresenceSettings(receipt, isInvisible, shareActivity);
        }
    }

    void TwitchSystemComponent::GetChannel(ReceiptID& receipt)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannel, ChannelInfoValue(ChannelInfo(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->GetChannel(receipt);
        }
    }
    
    void TwitchSystemComponent::GetChannelbyID(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelbyID, ChannelInfoValue(ChannelInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelbyID(receipt, channelID);
        }
    }

    void TwitchSystemComponent::UpdateChannel(ReceiptID& receipt, const ChannelUpdateInfo & channelUpdateInfo)
    {
        receipt.SetID(GetReceipt());

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdateChannel, ChannelInfoValue(ChannelInfo(), receipt, ResultCode::TwitchRESTError));
        }
        else
        {
            m_twitchREST->UpdateChannel(receipt, channelUpdateInfo);
        }
    }

    void TwitchSystemComponent::GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelEditors, UserInfoListValue(UserInfoList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelEditors(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelFollowers, FollowerResultValue(FollowerResult(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelFollowers(receipt, channelID, cursor, offset);
        }
    }

    void TwitchSystemComponent::GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelTeams, ChannelTeamValue(TeamInfoList(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelTeams(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelSubscribers, SubscriberValue(Subscription(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelSubscribers(receipt, channelID, offset);
        }
    }

    void TwitchSystemComponent::CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID) || !IsValidFriendID(userID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::CheckChannelSubscriptionbyUser, SubscriberbyUserValue(SubscriberInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->CheckChannelSubscriptionbyUser(receipt, channelID, userID);
        }
    }

    void TwitchSystemComponent::GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelVideos, VideoReturnValue(VideoReturn(), receipt, rc));
        }
        else
        {
            m_twitchREST->GetChannelVideos(receipt, channelID, boradcastType, language, offset);
        }
    }

    void TwitchSystemComponent::StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::StartChannelCommercial, StartChannelCommercialValue(StartChannelCommercialResult(), receipt, rc));
        }
        else
        {
            m_twitchREST->StartChannelCommercial(receipt, channelID, length);
        }
    }

    void TwitchSystemComponent::ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID)
    {
        receipt.SetID(GetReceipt());
        ResultCode rc(ResultCode::Success);

        if (m_cachedOAuthToken.empty() ||  m_twitchREST == nullptr)
        {
            rc = ResultCode::TwitchRESTError;
        }
        else if (!IsValidChannelID(channelID))
        {
            rc = ResultCode::InvalidParam;
        }

        if (rc != ResultCode::Success)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::ResetChannelStreamKey, ChannelInfoValue(ChannelInfo(), receipt, rc));
        }
        else
        {
            m_twitchREST->ResetChannelStreamKey(receipt, channelID);
        }
    }

    void TwitchSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TwitchService"));
    }

    void TwitchSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TwitchService"));
    }

    void TwitchSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void TwitchSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void TwitchSystemComponent::Init()
    {
#if AZ_TRAIT_TWITCH_INITIALIZE_SDK
        // You must define the Twitch Application Client ID
        m_twitchREST    = ITwitchREST::Alloc();

        // each time we create a interface we need a new session id, however this should not change during the life span of this object.
        AZ::Uuid sessionid(AZ::Uuid::Create());
        char temp[128];

        sessionid.ToString(temp, 128, false, false);

        m_cachedSessionID = AZStd::string(temp);
#endif // AZ_TRAIT_TWITCH_INITIALIZE_SDK
    }

    void TwitchSystemComponent::Activate()
    {
        TwitchRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void TwitchSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        TwitchRequestBus::Handler::BusDisconnect();
    }

    AZ::u64 TwitchSystemComponent::GetReceipt()
    {
        return ++m_receiptCounter;
    }

    bool TwitchSystemComponent::IsValidString(const AZStd::string& str, AZStd::size_t minLength, AZStd::size_t maxLength) const
    {
        bool isValid = false;

        /*
        ** From Twitch (Chris Adoretti <cadoretti@twitch.tv>) 3/14/17:
        **   I think its a safe bet though to make sure the string is alpha-numeric + dashes
        **   for now if that helps (0-9, a-f, A-F, -). We don't have a max length yet.
        **   The minimum length is 1.
        */

        if ((str.size() >= minLength) && (str.size() < maxLength))
        {
            AZStd::size_t found = str.find_first_not_of("0123456789abcdefABCDEF-");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }

    bool TwitchSystemComponent::IsValidTwitchAppID(const AZStd::string& twitchAppID) const
    {
        static const AZStd::size_t kMinIDLength = 24;   // min id length
        static const AZStd::size_t kMaxIDLength = 64;   // max id length
        bool isValid = false;

        if ((twitchAppID.size() >= kMinIDLength) && (twitchAppID.size() < kMaxIDLength))
        {
            AZStd::size_t found = twitchAppID.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");

            if (found == AZStd::string::npos)
            {
                isValid = true;
            }
        }

        return isValid;
    }
        
    bool TwitchSystemComponent::IsValidFriendID(const AZStd::string& friendID) const
    {
        // The min id length should be 1
        // The max id length will be huge, since there is no official max length, this will allow for a large id.

        return IsValidString(friendID, 1, 256);
    }

    bool TwitchSystemComponent::IsValidOAuthToken(const AZStd::string& oAuthToken) const
    {
        // Twitch OAuth tokens are exactly length 30

        if (oAuthToken.size() == 30)
        {
            AZStd::size_t found = oAuthToken.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");

            if (found == AZStd::string::npos)
            {
                return true;
            }
        }

        return false;
    }

    bool TwitchSystemComponent::IsValidSyncToken(const AZStd::string& syncToken) const
    {
        // sync tokens are either empty of a opaque token of a min length of 8, but no longer than 64

        return syncToken.empty() ? true : IsValidString(syncToken, 8, 64);
    }
}
