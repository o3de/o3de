/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <Twitch/TwitchBus.h>
#include <ITwitchREST.h>

namespace Twitch
{
    class TwitchSystemComponent
        : public AZ::Component
        , protected TwitchRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(TwitchSystemComponent, "{8AC76E51-CE55-4D67-90DE-41D1A7756E32}");

        TwitchSystemComponent();
        virtual ~TwitchSystemComponent() {}

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        void OnSystemTick() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // TwitchRequestBus interface implementation
        ////////////////////////////////////////////////////////////////////////

        void SetApplicationID(const AZStd::string& twitchApplicationID) override;
        void SetUserID(ReceiptID& receipt, const AZStd::string& userId) override;
        void SetOAuthToken(ReceiptID& receipt, const AZStd::string& token) override;
        void RequestUserID(ReceiptID& receipt) override;
        void RequestOAuthToken(ReceiptID& receipt) override;
        AZStd::string GetApplicationID() const override;
        AZStd::string GetUserID() const override;
        AZStd::string GetOAuthToken() const override;
        AZStd::string GetSessionID() const override;

        void GetUser(ReceiptID& receipt) override;
        void ResetFriendsNotificationCount(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendNotificationCount(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendRecommendations(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriends(ReceiptID & receipt, const AZStd::string& friendID, const AZStd::string& cursor) override;
        void GetFriendStatus(ReceiptID & receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID) override;
        void AcceptFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        void GetFriendRequests(ReceiptID & receipt, const AZStd::string& cursor) override;
        void CreateFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        void DeclineFriendRequest(ReceiptID & receipt, const AZStd::string& friendID) override;
        
        // presence
        void UpdatePresenceStatus(ReceiptID & receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext) override;
        void GetPresenceStatusofFriends(ReceiptID & receipt) override;
        void GetPresenceSettings(ReceiptID & receipt) override;
        void UpdatePresenceSettings(ReceiptID & receipt, bool isInvisible, bool shareActivity) override;
        
        // channel
        void GetChannel(ReceiptID& receipt) override;
        void GetChannelbyID(ReceiptID& receipt, const AZStd::string& channelID) override;
        void UpdateChannel(ReceiptID& receipt, const ChannelUpdateInfo& channelUpdateInfo) override;
        void GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset) override;
        void GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset) override;
        void CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID) override;
        void GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType broadcastType, const AZStd::string& language, AZ::u64 offset) override;
        void StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length) override;
        void ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        AZ::u64 GetReceipt();

    private:
        TwitchSystemComponent(const TwitchSystemComponent&) = delete;
        bool IsValidString(const AZStd::string& str, AZStd::size_t minLength, AZStd::size_t maxLength) const;
        bool IsValidFriendID(const AZStd::string& friendID) const;
        bool IsValidOAuthToken(const AZStd::string& oAuthToken) const;
        bool IsValidSyncToken(const AZStd::string& syncToken) const;
        bool IsValidChannelID(const AZStd::string& channelID) const { return IsValidFriendID(channelID); }
        bool IsValidCommunityID(const AZStd::string& communityID) const { return IsValidFriendID(communityID); }
        bool IsValidTwitchAppID(const AZStd::string& twitchAppID) const;

     private:
         AZStd::string           m_applicationID;
         AZStd::string           m_cachedClientID;
         AZStd::string           m_cachedOAuthToken;
         AZStd::string           m_cachedSessionID;
         AZ::u64                 m_refreshTokenExpireTime;

        ITwitchRESTPtr              m_twitchREST;
        AZStd::atomic<AZ::u64>      m_receiptCounter;
    };
}

