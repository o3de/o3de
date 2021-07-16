/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <HttpRequestor/HttpRequestorBus.h>

namespace Twitch
{
    class ITwitchREST;
    using ITwitchRESTPtr = AZStd::shared_ptr<ITwitchREST>;

    class ITwitchREST
    {
    public:
        static ITwitchRESTPtr Alloc();

        virtual ~ITwitchREST() = default;

        virtual void FlushEvents() = 0;

        /*
        ** Users
        */

        virtual void GetUser(ReceiptID& receipt) = 0;

        /*
        ** Friends
        */

        virtual void ResetFriendsNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendRecommendations(const ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriends(const ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor) = 0;
        virtual void GetFriendStatus(const ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID) = 0;
        virtual void AcceptFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void GetFriendRequests(const ReceiptID& receipt, const AZStd::string& cursor) = 0;
        virtual void CreateFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) = 0;
        virtual void DeclineFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) = 0;

        /*
        ** Presence
        */

        virtual void UpdatePresenceStatus(const ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext) = 0;
        virtual void GetPresenceStatusofFriends(const ReceiptID& receipt) = 0;
        virtual void GetPresenceSettings(const ReceiptID& receipt) = 0;
        virtual void UpdatePresenceSettings(const ReceiptID& receipt, bool isInvisible, bool shareActivity) = 0;

        /*
        ** Channels
        */

        virtual void GetChannel(const ReceiptID& receipt) = 0;
        virtual void GetChannelbyID(const ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void UpdateChannel(const ReceiptID& receipt, const ChannelUpdateInfo& channelUpdateInfo) = 0;
        virtual void GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset) = 0;
        virtual void GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID) = 0;
        virtual void GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset) = 0;
        virtual void CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID) = 0;
        virtual void GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset) = 0;
        virtual void StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length) = 0;
        virtual void ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID) = 0;

        /*
        ** Helper functions
        */

        virtual bool IsValidGameContext(const AZStd::string& gameContext) const = 0;
        virtual void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback) = 0;
        virtual void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback& callback) = 0;
    };
}
