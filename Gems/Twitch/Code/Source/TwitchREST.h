/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ITwitchREST.h"

namespace Twitch
{
    class TwitchREST : public ITwitchREST
    {
    private:
        using GetChannelCallback = AZStd::function<void(const ChannelInfo&, const ReceiptID&, ResultCode)>;

    public:
        TwitchREST();
        virtual ~TwitchREST() {}
        void FlushEvents() override;

        void GetUser(ReceiptID& receipt) override;
        
        void ResetFriendsNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID) override;
        void GetFriendNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID) override;
        void GetFriendRecommendations(const ReceiptID& receipt, const AZStd::string& friendID) override;
        void GetFriends(const ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor) override;
        void GetFriendStatus(const ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID) override;
        void AcceptFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) override;
        void GetFriendRequests(const ReceiptID& receipt, const AZStd::string& cursor) override;
        void CreateFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) override;
        void DeclineFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID) override;

        void UpdatePresenceStatus(const ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext) override;
        void GetPresenceStatusofFriends(const ReceiptID& receipt) override;
        void GetPresenceSettings(const ReceiptID& receipt) override;
        void UpdatePresenceSettings(const ReceiptID& receipt, bool isInvisible, bool shareActivity) override;

        void GetChannel(const ReceiptID& receipt) override;
        void GetChannelbyID(const ReceiptID& receipt, const AZStd::string& channelID) override;
        void UpdateChannel(const ReceiptID& receipt, const ChannelUpdateInfo& channelUpdateInfo) override;
        void GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset) override;
        void GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID) override;
        void GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset) override;
        void CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID) override;
        void GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset) override;
        void StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length) override;
        void ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID) override;

        bool IsValidGameContext(const AZStd::string& gameContext) const override;
        void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback) override;
        void AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback& callback) override;
        
    private:
        void InternalGetChannel(const ReceiptID& receipt, const GetChannelCallback& callback);
        AZStd::string BuildBaseURL(const AZStd::string& family, const AZStd::string& friendID = "") const;
        AZStd::string BuildKrakenURL(const AZStd::string& family) const;
        HttpRequestor::Headers GetDefaultHeaders();
        HttpRequestor::Headers GetClientIDHeader();
        void AddOAuthtHeader(HttpRequestor::Headers& headers);
        void AddClientIDHeader(HttpRequestor::Headers& headers);
        void AddAcceptToHeader(HttpRequestor::Headers& headers);
        void AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, const AZStd::string& key) const;
        void AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, AZ::s64 key) const;
        void AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, AZ::u64 key) const;
        AZ::u64 SafeGetUserInfoFromUserContainer(UserInfo& userInfo, const Aws::Utils::Json::JsonView& jsonInfo) const;
        AZ::u64 SafeGetUserInfo(UserInfo& userInfo, const Aws::Utils::Json::JsonView& jsonInfo) const;
        bool SafeGetJSONString(AZStd::string& value, const char*key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONu64(AZ::u64& value, const char*key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONs64(AZ::s64& value, const char*key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONbool(bool& value, const char*key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONdouble(double& value, const char*key, const Aws::Utils::Json::JsonView& json) const;
        AZ::u64 SafeGetUserNotifications(UserNotifications& iserNotifications, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetPresenceActivityType(PresenceActivityType& activityType, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetPresenceAvailability(PresenceAvailability& availability, const Aws::Utils::Json::JsonView& json) const;
        AZ::u64 SafeGetChannelInfo(ChannelInfo& channelInfo, const Aws::Utils::Json::JsonView& json) const;
        AZ::u64 SafeGetTeamInfo(TeamInfo& teamInfo, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONBroadCastType(BroadCastType& type, const char*key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoChannel(VideoChannelInfo& channelInfo, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoFPS(FPSInfo& fps, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoPreview(PreviewInfo& preview, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoResolutions(ResolutionsInfo& resolutions, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoThumbnailInfo(ThumbnailInfo& info, const char *key, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetJSONVideoThumbnails(ThumbnailsInfo& thumbnails, const Aws::Utils::Json::JsonView& json) const;
        bool SafeGetChannelCommunityInfo(CommunityInfo & info, const Aws::Utils::Json::JsonView& json) const;

        AZStd::string GetPresenceAvailabilityName(PresenceAvailability availability) const;
        AZStd::string GetPresenceActivityTypeName(PresenceActivityType activityType) const;
        PresenceAvailability GetPresenceAvailability(const AZStd::string& name) const;
        PresenceActivityType GetPresenceActivityType(const AZStd::string& name) const;
        AZStd::string GetBroadCastTypeNameFromType(BroadCastType type) const;
        BroadCastType GetBroadCastTypeFromName(const AZStd::string& name) const;
        AZ::u64 GetComercialLength(CommercialLength lenght) const;

    private:
        static const char * kProtocol;       // protocol to use, typically https
        static const char * kBasePath;       // base path for the Twitch API
        static const char * kVer;            // version for the Twitch API
        static const char * kKraken;         // the name for the kraken api
        static const char * kAuthType;       // Authorization type
        static const char * kAcceptType;     // Accept type (and version) 

        using PresenceAvailabilityMap = AZStd::map<PresenceAvailability, AZStd::string>;
        using PresenceActivityTypeNameMap = AZStd::map<PresenceActivityType, AZStd::string>;

    private:
        PresenceAvailabilityMap         m_availabilityMap;
        PresenceActivityTypeNameMap     m_activityTypeMap;
    };
}

