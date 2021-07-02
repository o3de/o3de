/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Twitch_precompiled.h"
#include <AzCore/std/time.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <Twitch/TwitchBus.h>
#include <HttpRequestor/HttpRequestorBus.h>
#include "TwitchREST.h"

namespace Twitch
{
    ITwitchRESTPtr ITwitchREST::Alloc()
    {
        return AZStd::make_shared<TwitchREST>();
    }

    const char * TwitchREST::kProtocol("https");
    const char * TwitchREST::kBasePath("api.twitch.tv");
    const char * TwitchREST::kVer("v5");
    const char * TwitchREST::kKraken("kraken");
    const char * TwitchREST::kAuthType("OAuth ");
    const char * TwitchREST::kAcceptType("application/vnd.twitchtv.v5+json");

    TwitchREST::TwitchREST()
    {
        // all names listed below comply with Twitch's naming rules, Do not change the case or 
        // spelling of the return values! Also do not put in the ::Unknown strings, placehold only!

        m_availabilityMap[PresenceAvailability::Idle]           = "idle";
        m_availabilityMap[PresenceAvailability::Online]         = "online";
        
        m_activityTypeMap[PresenceActivityType::Watching]       = "watching";
        m_activityTypeMap[PresenceActivityType::Playing]        = "playing";
        m_activityTypeMap[PresenceActivityType::Broadcasting]   = "broadcasting";
    }

    void TwitchREST::FlushEvents()
    {
        TwitchNotifyBus::ExecuteQueuedEvents();
    }

    void TwitchREST::GetUser(ReceiptID& receipt)
    {
        AZStd::string url( BuildKrakenURL("user") );

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& json, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            UserInfo userinfo;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetUserInfo(userinfo, json);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetUser, UserInfoValue(userinfo, receipt, rc));
        });
    }
    
    void TwitchREST::ResetFriendsNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url( BuildBaseURL("users", friendID) + "/friends/notifications");
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& /*json*/, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);

            if (httpCode == Aws::Http::HttpResponseCode::NO_CONTENT)    // 204: NO_CONTENT The server successfully processed the request and is not returning any content
            {
                rc = ResultCode::Success;
            }
        
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::ResetFriendsNotificationCountNotify, Int64Value(static_cast<AZ::s64>(httpCode), receipt, rc));
        });
    }
    

    void TwitchREST::GetFriendNotificationCount(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url(BuildBaseURL("users", friendID) + "/friends/notifications");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& json, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            AZ::s64 count = 0;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;
                count = json.GetInteger("count");
            }
            else
            {
                count = static_cast<AZ::s64>(httpCode);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendNotificationCount, Int64Value(count, receipt, rc));
        });
    }

    void TwitchREST::GetFriendRecommendations(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url(BuildBaseURL("users", friendID) + "/friends/recommendations");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            FriendRecommendationList returnRecommendations;
            
            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;
                Aws::Utils::Array<Aws::Utils::Json::JsonView> recommendations = jsonDoc.GetArray("recommendations");

                for(size_t index =0; index<recommendations.GetLength(); index++)
                {
                    FriendRecommendation fr;

                    Aws::Utils::Json::JsonView item = recommendations.GetItem(index);

                    fr.Reason = item.GetString("reason").c_str();
                    SafeGetUserInfoFromUserContainer(fr.User, item);

                    returnRecommendations.push_back(fr);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendRecommendations, FriendRecommendationValue(returnRecommendations, receipt, rc));
        });
    }

    void TwitchREST::GetFriends(const ReceiptID& receipt, const AZStd::string& friendID, const AZStd::string& cursor)
    {
        AZStd::string url(BuildBaseURL("users", friendID) + "/friends/relationships");

        HttpRequestor::Headers headers( GetDefaultHeaders() );

        AddToHeader(headers, "limit", 256ULL);

        if( !cursor.empty() )
            AddToHeader(headers, "cursor", cursor);
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, headers, [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            GetFriendReturn friendReturn;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONString(friendReturn.Cursor, "cursor", jsonDoc);

                Aws::Utils::Array<Aws::Utils::Json::JsonView> recommendations = jsonDoc.GetArray("friends");

                for (size_t index = 0; index < recommendations.GetLength(); index++)
                {
                    FriendInfo fi;

                    Aws::Utils::Json::JsonView item = recommendations.GetItem(index);

                    SafeGetJSONString(fi.CreatedDate, "created_at", item);
                    SafeGetUserInfoFromUserContainer(fi.User, item);

                    friendReturn.Friends.push_back(fi);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriends, GetFriendValue(friendReturn, receipt, rc));
        });
    }

    void TwitchREST::GetFriendStatus(const ReceiptID& receipt, const AZStd::string& sourceFriendID, const AZStd::string& targetFriendID)
    {
        AZStd::string url(BuildBaseURL("users", sourceFriendID) + "/friends/relationships/" + targetFriendID);

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            FriendStatus friendStatus;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONString(friendStatus.Status, "status", jsonDoc);
                SafeGetUserInfoFromUserContainer(friendStatus.User, jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendStatus, FriendStatusValue(friendStatus, receipt, rc));
        });
    }

    void TwitchREST::AcceptFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url(BuildBaseURL("users") + "/friends/relationships/" + friendID);

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_PUT, GetDefaultHeaders(), [receipt, this]([[maybe_unused]] const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);

            if (httpCode == Aws::Http::HttpResponseCode::CREATED)
            {
                rc = ResultCode::Success;
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::AcceptFriendRequest, Int64Value(static_cast<AZ::s64>(httpCode), receipt, rc));
        });
    }


    void TwitchREST::GetFriendRequests(const ReceiptID& receipt, const AZStd::string& cursor)
    {
        AZStd::string url(BuildBaseURL("users") + "/friends/requests");

        HttpRequestor::Headers headers(GetDefaultHeaders());

        AddToHeader(headers, "limit", 512ULL);

        if (!cursor.empty())
            AddToHeader(headers, "cursor", cursor);

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, headers, [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            FriendRequestResult requestResult;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONString(requestResult.Cursor, "cursor", jsonDoc);
                SafeGetJSONu64(requestResult.Total, "total", jsonDoc);

                Aws::Utils::Array<Aws::Utils::Json::JsonView> recommendations = jsonDoc.GetArray("requests");

                for (size_t index = 0; index < recommendations.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = recommendations.GetItem(index);
                    FriendRequest fr;

                    SafeGetJSONbool(fr.IsRecommended, "is_recommended", item);
                    SafeGetJSONbool(fr.IsStranger, "is_stranger", item);
                    SafeGetJSONString(fr.NonStrangerReason, "non_stranger_reason", item);
                    SafeGetJSONString(fr.RequestedDate, "requested_at", item);
                    SafeGetUserInfoFromUserContainer(fr.User, item);

                    requestResult.Requests.push_back(fr);
                }
            }
            
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetFriendRequests, FriendRequestValue(requestResult, receipt, rc));
        });
    }

    void TwitchREST::CreateFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url(BuildBaseURL("users") + "/friends/requests/" + friendID);

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_PUT, GetDefaultHeaders(), [receipt, this]([[maybe_unused]] const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);

            if (httpCode == Aws::Http::HttpResponseCode::CREATED)
            {
                rc = ResultCode::Success;
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::CreateFriendRequest, Int64Value(static_cast<AZ::s64>(httpCode), receipt, rc));
        });
    }

    void TwitchREST::DeclineFriendRequest(const ReceiptID& receipt, const AZStd::string& friendID)
    {
        AZStd::string url(BuildBaseURL("users") + "/friends/requests/" + friendID);

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, GetDefaultHeaders(), [receipt, this]([[maybe_unused]] const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);

            if (httpCode == Aws::Http::HttpResponseCode::CREATED)
            {
                rc = ResultCode::Success;
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::DeclineFriendRequest, Int64Value(static_cast<AZ::s64>(httpCode), receipt, rc));
        });
    }
    
    void TwitchREST::UpdatePresenceStatus(const ReceiptID& receipt, PresenceAvailability availability, PresenceActivityType activityType, const AZStd::string& gameContext)
    {
        // we need to get the twitch channel this user is on, and that call requires its own receipt
        ReceiptID gcReceipt;

        InternalGetChannel(gcReceipt, [receipt, availability, activityType, gameContext, this](const ChannelInfo& channelInfo, const ReceiptID&, ResultCode)
        {
            AZStd::string url(BuildBaseURL("users") + "/status");

            HttpRequestor::Headers headers(GetDefaultHeaders());
            AddToHeader(headers, "Content-Type", "application/json");
        
            AZStd::string appID;
            TwitchRequestBus::BroadcastResult(appID, &TwitchRequests::GetApplicationID);

            Aws::Utils::Json::JsonValue jsonActivity;
            jsonActivity.WithString("type", GetPresenceActivityTypeName(activityType).c_str());
            jsonActivity.WithString("channel_id", channelInfo.Id.c_str());
            jsonActivity.WithString("game_id", appID.c_str());

            if ( (activityType == PresenceActivityType::Playing) && !gameContext.empty() )
            {
                Aws::Utils::Json::JsonValue jsonGameContext(Aws::String(gameContext.c_str()));

                if( jsonGameContext.WasParseSuccessful() )
                    jsonActivity.WithObject("game_context", AZStd::move(jsonGameContext));
            }
        
            AZStd::string sessionID;
            TwitchRequestBus::BroadcastResult(sessionID, &TwitchRequests::GetSessionID);

            Aws::Utils::Json::JsonValue jsonBody;
            jsonBody.WithString("session_id", sessionID.c_str());
            jsonBody.WithString("availability", GetPresenceAvailabilityName(availability).c_str());
            jsonBody.WithObject("activities", AZStd::move(jsonActivity));

            Aws::String body(jsonBody.View().WriteCompact());
                
            AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_POST, headers, body.c_str(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRESTError);
                AZ::s64 pollIntervalSeconds = 0;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;

                    SafeGetJSONs64(pollIntervalSeconds, "poll_interval_seconds", jsonDoc);
                }
                else
                {
                    pollIntervalSeconds = static_cast<AZ::s64>(httpCode);
                }

                TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdatePresenceStatus, Int64Value(pollIntervalSeconds, receipt, rc));
            });
        });
    }

    void TwitchREST::GetPresenceStatusofFriends(const ReceiptID& receipt)
    {
        AZStd::string url(BuildBaseURL("users") + "/status/friends");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            PresenceStatusList statusList;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                Aws::Utils::Array<Aws::Utils::Json::JsonView> recommendations = jsonDoc.GetArray("data");

                for (size_t index = 0; index < recommendations.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = recommendations.GetItem(index);
                    PresenceStatus ps;

                    SafeGetJSONs64(ps.Index, "index", item);
                    SafeGetJSONs64(ps.UpdatedDate, "UpdatedDate", item);
                    SafeGetJSONString(ps.UserID, "user_id", item);
                    SafeGetPresenceActivityType(ps.ActivityType, item);
                    SafeGetPresenceAvailability(ps.Availability, item);

                    statusList.push_back(ps);
                }
            }
            
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetPresenceStatusofFriends, PresenceStatusValue(statusList, receipt, rc));
        });
    }

    void TwitchREST::GetPresenceSettings(const ReceiptID& receipt)
    {
        AZStd::string url(BuildBaseURL("users") + "/status/settings");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            PresenceSettings presenceSettings;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONbool(presenceSettings.IsInvisible, "is_invisible", jsonDoc);
                SafeGetJSONbool(presenceSettings.ShareActivity, "share_activity", jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetPresenceSettings, PresenceSettingsValue(presenceSettings, receipt, rc));
        });
    }

    void TwitchREST::UpdatePresenceSettings(const ReceiptID& receipt, bool isInvisible, bool shareActivity)
    {
        AZStd::string url(BuildBaseURL("users") + "/status/settings");

        HttpRequestor::Headers headers(GetDefaultHeaders());
        AddToHeader(headers, "Content-Type", "application/json");

        Aws::Utils::Json::JsonValue jsonBody;
        jsonBody.WithBool("is_invisible", isInvisible);
        jsonBody.WithBool("share_activity", shareActivity);

        Aws::String body(jsonBody.View().WriteCompact());

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_POST, headers, body.c_str(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            PresenceSettings presenceSettings;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONbool(presenceSettings.IsInvisible, "is_invisible", jsonDoc);
                SafeGetJSONbool(presenceSettings.ShareActivity, "share_activity", jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdatePresenceSettings, PresenceSettingsValue(presenceSettings, receipt, rc));
        });
    }

    void TwitchREST::GetChannel(const ReceiptID& receipt)
    {
        InternalGetChannel(receipt, [](const ChannelInfo& channelInfo, const ReceiptID& receipt, ResultCode rc)
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannel, ChannelInfoValue(channelInfo, receipt, rc));
        });
    }

    void TwitchREST::GetChannelbyID(const ReceiptID& receipt, const AZStd::string& channelID)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + (channelID.empty() ? "0000000" : channelID));

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetClientIDHeader(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            ChannelInfo channelInfo;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                channelInfo.NumItemsRecieved = SafeGetChannelInfo(channelInfo, jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelbyID, ChannelInfoValue(channelInfo, receipt, rc));
        });
    }


    void TwitchREST::UpdateChannel(const ReceiptID& receipt, const ChannelUpdateInfo& channelUpdateInfo)
    {
        /*
        ** sanity check here, at least once of these must be set to update.
        */

        if( ( !channelUpdateInfo.ChannelFeedEnabled.ToBeUpdated() ) &&
            ( !channelUpdateInfo.Delay.ToBeUpdated()) &&
            ( !channelUpdateInfo.GameName.ToBeUpdated()) &&
            ( !channelUpdateInfo.Status.ToBeUpdated()))
        {
            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdateChannel, ChannelInfoValue(ChannelInfo(), receipt, ResultCode::TwitchChannelNoUpdatesToMake));
        }

        // we need to get the twitch channel this user is on, and that call requires its own receipt
        ReceiptID gcReceipt;
        
        InternalGetChannel(gcReceipt, [receipt, channelUpdateInfo, this](const ChannelInfo& channelInfo, const ReceiptID&, ResultCode)
        {
            AZStd::string url(BuildKrakenURL("channels") + "/" + channelInfo.Id);

            HttpRequestor::Headers headers(GetDefaultHeaders());                  
            AddToHeader(headers, "Content-Type", "application/json");

            Aws::Utils::Json::JsonValue jsonChannel;

            if (channelUpdateInfo.Status.ToBeUpdated())
            {
                jsonChannel.WithString("status", channelUpdateInfo.Status.GetValue().c_str());
            }

            if (channelUpdateInfo.GameName.ToBeUpdated())
            {
                jsonChannel.WithString("game", channelUpdateInfo.GameName.GetValue().c_str());
            }
        
            if (channelUpdateInfo.Delay.ToBeUpdated())
            {
                jsonChannel.WithString("delay", AZStd::to_string(channelUpdateInfo.Delay.GetValue()).c_str());
            }
            
            if (channelUpdateInfo.ChannelFeedEnabled.ToBeUpdated())
            {
                jsonChannel.WithBool("channel_feed_enabled", channelUpdateInfo.ChannelFeedEnabled.GetValue());
            }
            
            Aws::Utils::Json::JsonValue jsonBody;

            jsonBody.WithObject("channel", AZStd::move(jsonChannel));

            Aws::String body(jsonBody.View().WriteCompact());

            AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_PUT, headers, body.c_str(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
            {
                ResultCode rc(ResultCode::TwitchRESTError);
                ChannelInfo retChannelInfo;

                if (httpCode == Aws::Http::HttpResponseCode::OK)
                {
                    rc = ResultCode::Success;

                    retChannelInfo.NumItemsRecieved = SafeGetChannelInfo(retChannelInfo, jsonDoc);
                }

                TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::UpdateChannel, ChannelInfoValue(retChannelInfo, receipt, rc));
            });
        });
    }

    void TwitchREST::GetChannelEditors(ReceiptID& receipt, const AZStd::string& channelID)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/editors");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            UserInfoList userList;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonUserArray = jsonDoc.GetArray("users");

                for (size_t index = 0; index < jsonUserArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonUserArray.GetItem(index);
                    UserInfo ui;

                    SafeGetUserInfo(ui, item);

                    userList.push_back(ui);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelEditors, UserInfoListValue(userList, receipt, rc));
        });
    }
    
    void TwitchREST::GetChannelFollowers(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& cursor, AZ::u64 offset)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/follows?limit=100");

        if( !cursor.empty() )
        {
            url += "&cursor=";
            url += cursor;
            url += "&offset=";
            url += AZStd::to_string(offset);
        }

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetClientIDHeader(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            FollowerResult followerResult;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONString(followerResult.Cursor, "_cursor", jsonDoc);
                SafeGetJSONu64(followerResult.Total, "_total", jsonDoc);

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonFollowsArray = jsonDoc.GetArray("follows");

                for (size_t index = 0; index < jsonFollowsArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonFollowsArray.GetItem(index);
                    Follower follower;
                    
                    SafeGetJSONString(follower.CreatedDate, "created_at", item);
                    SafeGetJSONbool(follower.Notifications, "notifications", item);
                    SafeGetUserInfoFromUserContainer(follower.User, item);

                    followerResult.Followers.push_back(follower);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelFollowers, FollowerResultValue(followerResult, receipt, rc));
        });
    }

    void TwitchREST::GetChannelTeams(ReceiptID& receipt, const AZStd::string& channelID)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/teams");
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetClientIDHeader(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            TeamInfoList teamInfoList;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonArray = jsonDoc.GetArray("teams");

                for (size_t index = 0; index < jsonArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonArray.GetItem(index);
                    TeamInfo teamInfo;

                    SafeGetTeamInfo(teamInfo, item);

                    teamInfoList.push_back(teamInfo);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelTeams, ChannelTeamValue(teamInfoList, receipt, rc));
        });
    }

    void TwitchREST::GetChannelSubscribers(ReceiptID& receipt, const AZStd::string& channelID, AZ::u64 offset)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/subscriptions?limit=100");

        if (offset > 0)
        {
            url += "&offset=";
            url += AZStd::to_string(offset);
        }

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            Subscription subscription;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONu64(subscription.Total, "_total", jsonDoc);

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonSubscriptionsArray = jsonDoc.GetArray("subscriptions");

                for (size_t index = 0; index < jsonSubscriptionsArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonSubscriptionsArray.GetItem(index);
                    SubscriberInfo si;

                    SafeGetJSONString(si.ID, "_id", item);
                    SafeGetJSONString(si.CreatedDate, "created_at", item);
                    SafeGetUserInfoFromUserContainer(si.User, item);

                    subscription.Subscribers.push_back(si);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelSubscribers, SubscriberValue(subscription, receipt, rc));
        });
    }

    void TwitchREST::CheckChannelSubscriptionbyUser(ReceiptID& receipt, const AZStd::string& channelID, const AZStd::string& userID)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/subscriptions/" + userID);
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            SubscriberInfo si;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONString(si.ID, "_id", jsonDoc);
                SafeGetJSONString(si.CreatedDate, "created_at", jsonDoc);
                SafeGetUserInfoFromUserContainer(si.User, jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::CheckChannelSubscriptionbyUser, SubscriberbyUserValue(si, receipt, rc));
        });
    }

    void TwitchREST::GetChannelVideos(ReceiptID& receipt, const AZStd::string& channelID, BroadCastType boradcastType, const AZStd::string& language, AZ::u64 offset)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/videos?limit=100");

        if (offset > 0)
        {
            url += "&offset=";
            url += AZStd::to_string(offset);
        }

        AZStd::string bt( GetBroadCastTypeNameFromType(boradcastType) );
        if( !bt.empty() )
        {
            url += "&broadcast_type=";
            url += bt;
        }

        if( !language.empty() )
        {
            url += "&language=";
            url += language;
        
        }
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            VideoReturn videoReturn;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONu64(videoReturn.Total, "_total", jsonDoc);

                Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonVideosArray = jsonDoc.GetArray("videos");

                for (size_t index = 0; index < jsonVideosArray.GetLength(); index++)
                {
                    Aws::Utils::Json::JsonView item = jsonVideosArray.GetItem(index);
                    VideoInfo vi;

                    SafeGetJSONString(vi.ID, "_id", item);
                    SafeGetJSONu64(vi.BroadcastID, "broadcast_id", item);
                    SafeGetJSONBroadCastType(vi.Type, "broadcast_type", item);
                    SafeGetJSONVideoChannel(vi.Channel, item);
                    SafeGetJSONString(vi.CreatedDate, "created_at", item);
                    SafeGetJSONString(vi.Description, "description", item);
                    SafeGetJSONString(vi.DescriptionHTML, "description_html", item);
                    SafeGetJSONVideoFPS(vi.FPS, item);
                    SafeGetJSONString(vi.Game, "game", item);
                    SafeGetJSONString(vi.Language, "language", item);
                    SafeGetJSONu64(vi.Length, "length", item);
                    SafeGetJSONVideoPreview(vi.Preview, item);
                    SafeGetJSONString(vi.PublishedDate, "published_at", item);
                    SafeGetJSONVideoResolutions(vi.Resolutions, item);
                    SafeGetJSONString(vi.Status, "status", item);
                    SafeGetJSONString(vi.TagList, "tag_list", item);
                    SafeGetJSONVideoThumbnails(vi.Thumbnails, item);
                    SafeGetJSONString(vi.Title, "title", item);
                    SafeGetJSONString(vi.URL, "url", item);
                    SafeGetJSONString(vi.Viewable, "viewable", item);
                    SafeGetJSONString(vi.ViewableAt, "viewable_at", item);
                    SafeGetJSONu64(vi.Views, "views", item);

                    videoReturn.Videos.push_back(vi);
                }
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::GetChannelVideos, VideoReturnValue(videoReturn, receipt, rc));
        });
    }


    void TwitchREST::StartChannelCommercial(ReceiptID& receipt, const AZStd::string& channelID, CommercialLength length)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/commercial");

        HttpRequestor::Headers headers( GetDefaultHeaders() );
        AddToHeader(headers, "Content-Type", "application/json");

        Aws::Utils::Json::JsonValue jsonBody;
        jsonBody.WithInt64("duration", GetComercialLength(length) );

        Aws::String body(jsonBody.View().WriteCompact());
        
        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_POST, headers, body.c_str(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            StartChannelCommercialResult cr;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetJSONu64(cr.Duration, "duration", jsonDoc);
                SafeGetJSONString(cr.Message, "message", jsonDoc);
                SafeGetJSONu64(cr.RetryAfter, "retryafter", jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::StartChannelCommercial, StartChannelCommercialValue(cr, receipt, rc));
        });
    }

    void TwitchREST::ResetChannelStreamKey(ReceiptID& receipt, const AZStd::string& channelID)
    {
        AZStd::string url(BuildKrakenURL("channels") + "/" + channelID + "/stream_key");

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_DELETE, GetDefaultHeaders(), [receipt, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            ChannelInfo ci;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                SafeGetChannelInfo(ci, jsonDoc);
            }

            TwitchNotifyBus::QueueBroadcast(&TwitchNotifyBus::Events::ResetChannelStreamKey, ChannelInfoValue(ci, receipt, rc));
        });
    }

    bool TwitchREST::IsValidGameContext(const AZStd::string& gameContext) const
    {
        bool isValid = false;

        /*
        ** Insure gameContext is a valid JSON object, and that means there must be a string!
        */

        if( !gameContext.empty() )
        {
            Aws::Utils::Json::JsonValue json(Aws::String(gameContext.c_str()));

            isValid = json.WasParseSuccessful();
        }
        
        return isValid;
    }

    void TwitchREST::AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const HttpRequestor::Callback & callback)
    {
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeaders, URI, method, headers, callback);
    }

    void TwitchREST::AddHTTPRequest(const AZStd::string& URI, Aws::Http::HttpMethod method, const HttpRequestor::Headers & headers, const AZStd::string& body, const HttpRequestor::Callback & callback)
    {
        HttpRequestor::HttpRequestorRequestBus::Broadcast(&HttpRequestor::HttpRequestorRequests::AddRequestWithHeadersAndBody, URI, method, headers, body, callback);
    }
        
    void TwitchREST::InternalGetChannel(const ReceiptID& receipt, const GetChannelCallback& callback)
    {
        AZStd::string url(BuildKrakenURL("channel"));

        AddHTTPRequest(url, Aws::Http::HttpMethod::HTTP_GET, GetDefaultHeaders(), [receipt, callback, this](const Aws::Utils::Json::JsonView& jsonDoc, Aws::Http::HttpResponseCode httpCode)
        {
            ResultCode rc(ResultCode::TwitchRESTError);
            ChannelInfo channelInfo;

            if (httpCode == Aws::Http::HttpResponseCode::OK)
            {
                rc = ResultCode::Success;

                channelInfo.NumItemsRecieved = SafeGetChannelInfo(channelInfo, jsonDoc);
            }

            callback(channelInfo, receipt, rc);
        });
    }

    AZStd::string TwitchREST::BuildBaseURL(const AZStd::string& family, const AZStd::string& friendID /*= ""*/) const
    {
        /*
        ** return a URL something like https://api.twitch.tv/v5/<family>/<clientid>
        */

        AZStd::string url(kProtocol);
        AZStd::string clientID;

        // if user id is empty, then get the current client id for the logged in user.
        if( friendID.empty() )
        {
            TwitchRequestBus::BroadcastResult(clientID, &TwitchRequests::GetUserID);
        }
        else
        {
            clientID = friendID;
        }

        url += "://";
        url += kBasePath;           // kBasePath("api.twitch.tv");
        url += "/";
        url += kVer;                // kVer("v5");
        url += "/";
        url += family;
        url += "/";
        url += clientID;

        return url;
    }

    AZStd::string TwitchREST::BuildKrakenURL(const AZStd::string& family) const
    {
        /*
        ** return a URL something like https://api.twitch.tv/kraken/<family>
        */

        AZStd::string url(kProtocol);

        url += "://";
        url += kBasePath;           // kBasePath("api.twitch.tv");
        url += "/";
        url += kKraken;             // kKraken("kraken");
        url += "/";
        url += family;

        return url;
    }
    
    HttpRequestor::Headers TwitchREST::GetDefaultHeaders()
    {
        HttpRequestor::Headers hdrs;
        
        AddAcceptToHeader(hdrs);
        AddOAuthtHeader(hdrs);
        AddClientIDHeader(hdrs);

        return hdrs;
    }

    HttpRequestor::Headers TwitchREST::GetClientIDHeader()
    {
        HttpRequestor::Headers hdrs;

        AddAcceptToHeader(hdrs);
        AddClientIDHeader(hdrs);

        return hdrs;
    }

    void TwitchREST::AddOAuthtHeader(HttpRequestor::Headers& headers)
    {
        AZStd::string oAuthToken;
        TwitchRequestBus::BroadcastResult(oAuthToken, &TwitchRequests::GetOAuthToken);
        headers["Authorization"] = kAuthType + oAuthToken;
    }
    
    // return the application id in a header, the rest docs refer this as the client-id, poorly named)
    void TwitchREST::AddClientIDHeader(HttpRequestor::Headers& headers)
    {
        AZStd::string appID;
        TwitchRequestBus::BroadcastResult(appID, &TwitchRequests::GetApplicationID);
        headers["Client-ID"] = appID;
    }

    void TwitchREST::AddAcceptToHeader(HttpRequestor::Headers& headers)
    {
        headers["Accept"] = kAcceptType;
    }

    void TwitchREST::AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, const AZStd::string& key) const
    {
        headers[name] = key;
    }

    void TwitchREST::AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, AZ::s64 key) const
    {
        AddToHeader(headers, name, AZStd::to_string(key));    
    }

    void TwitchREST::AddToHeader(HttpRequestor::Headers& headers, const AZStd::string& name, AZ::u64 key) const
    {
        AddToHeader(headers, name, AZStd::to_string(key));
    }
    
    AZ::u64 TwitchREST::SafeGetUserInfoFromUserContainer(UserInfo& userInfo, const Aws::Utils::Json::JsonView& jsonInfo) const
    {
        AZ::u64 itemCount = 0;

        if (jsonInfo.ValueExists("user") )
        {
            Aws::Utils::Json::JsonView jsonUser(jsonInfo.GetObject("user"));

            itemCount = SafeGetUserInfo(userInfo, jsonUser);
        }

        return itemCount;
    }

    AZ::u64 TwitchREST::SafeGetUserInfo(UserInfo& userInfo, const Aws::Utils::Json::JsonView& jsonInfo) const
    {
        AZ::u64 itemCount = 0;

        itemCount += SafeGetJSONString(userInfo.ID, "_id", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.Bio, "bio", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.CreatedDate, "created_at", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.DisplayName, "display_name", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.EMail, "email", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.Logo, "logo", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.Name, "name", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.ProfileBanner, "profile_banner", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.ProfileBannerBackgroundColor, "profile_banner_background_color", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.Type, "type", jsonInfo);
        itemCount += SafeGetJSONString(userInfo.UpdatedDate, "updated_at", jsonInfo);
        itemCount += SafeGetJSONbool(userInfo.EMailVerified, "email_verified", jsonInfo); 
        itemCount += SafeGetJSONbool(userInfo.Partnered, "partnered", jsonInfo); 
        itemCount += SafeGetJSONbool(userInfo.TwitterConnected, "twitter_connected", jsonInfo);
        itemCount += SafeGetUserNotifications(userInfo.Notifications, jsonInfo);

        return itemCount;
    }

    bool TwitchREST::SafeGetJSONString(AZStd::string& value, const char *key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;
        if ( (key != nullptr)&& json.ValueExists(key) )
        {
            success = true;
            value = json.GetString(key).c_str();
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONu64(AZ::u64& value, const char *key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;

        if ((key != nullptr)&& json.ValueExists(key))
        {
            success = true;
            value = static_cast<AZ::u64>(json.GetInt64(key));
        }

        return success;
    }
    
    bool TwitchREST::SafeGetJSONs64(AZ::s64& value, const char *key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;

        if ((key != nullptr)&& json.ValueExists(key))
        {
            success = true;
            value = json.GetInt64(key);
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONbool(bool& value, const char *key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;

        if ((key != nullptr)&& json.ValueExists(key))
        {
            success = true;
            value = json.GetBool(key);
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONdouble(double& value, const char* key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;

        if ((key != nullptr) && json.ValueExists(key))
        {
            success = true;
            value = json.GetDouble(key);
        }

        return success;
    }

    AZ::u64 TwitchREST::SafeGetUserNotifications(UserNotifications& userNotifications, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json value contains:
        // "notifications": { "email": false, "push" : true }
        AZ::u64 numItems = 0;

        if (json.ValueExists("notifications"))
        {
            Aws::Utils::Json::JsonView jsonNotifications(json.GetObject("notifications"));

            numItems += SafeGetJSONbool(userNotifications.EMail, "email", jsonNotifications);
            numItems += SafeGetJSONbool(userNotifications.Push, "push", jsonNotifications);
        }

        return numItems;
    }
        
    bool TwitchREST::SafeGetPresenceActivityType(PresenceActivityType& activityType, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;
        AZStd::string name;

        if (SafeGetJSONString(name, "availability", json))
        {
            activityType = GetPresenceActivityType(name);
            success = true;
        }
            
        return success;
    }

    bool TwitchREST::SafeGetPresenceAvailability(PresenceAvailability& availability, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "activity": { "type": "none"}
       
        bool success = false;
        
        if( json.ValueExists("activity") )
        {
            Aws::Utils::Json::JsonView jsonActivity( json.GetObject("activity") );
            AZStd::string typeName;
            if( SafeGetJSONString(typeName, "type", jsonActivity) )
            {
                availability = GetPresenceAvailability(typeName);

                success = true;
            }
        }

        return success;
    }

    AZ::u64 TwitchREST::SafeGetChannelInfo(ChannelInfo& channelInfo, const Aws::Utils::Json::JsonView& json) const
    {
        AZ::u64 itemCount = 0;
        
        itemCount += SafeGetJSONString(channelInfo.Id, "_id", json);
        itemCount += SafeGetJSONString(channelInfo.BroadcasterLanguage, "broadcaster_language", json);
        itemCount += SafeGetJSONString(channelInfo.CreatedDate, "created_at", json);
        itemCount += SafeGetJSONString(channelInfo.DisplayName, "display_name", json);
        itemCount += SafeGetJSONString(channelInfo.eMail, "email", json);                       // only returned when invoked via GetChannel
        itemCount += SafeGetJSONu64(channelInfo.NumFollowers, "followers", json);
        itemCount += SafeGetJSONString(channelInfo.GameName, "game", json);
        itemCount += SafeGetJSONString(channelInfo.Lanugage, "language", json);
        itemCount += SafeGetJSONString(channelInfo.Logo, "logo", json);
        itemCount += SafeGetJSONbool(channelInfo.Mature, "mature", json);
        itemCount += SafeGetJSONString(channelInfo.Name, "name", json);
        itemCount += SafeGetJSONbool(channelInfo.Partner, "partner", json);
        itemCount += SafeGetJSONString(channelInfo.ProfileBanner, "profile_banner", json);
        itemCount += SafeGetJSONString(channelInfo.ProfileBannerBackgroundColor, "profile_banner_background_color", json);
        itemCount += SafeGetJSONString(channelInfo.Status, "status", json);
        itemCount += SafeGetJSONString(channelInfo.StreamKey, "stream_key", json);              // only returned when invoked via GetChannel
        itemCount += SafeGetJSONString(channelInfo.UpdatedDate, "updated_at", json);
        itemCount += SafeGetJSONString(channelInfo.URL, "url", json);
        itemCount += SafeGetJSONString(channelInfo.VideoBanner, "video_banner", json);
        itemCount += SafeGetJSONu64(channelInfo.NumViews, "views", json);

        return itemCount;
    }

    AZ::u64 TwitchREST::SafeGetTeamInfo(TeamInfo& teamInfo, const Aws::Utils::Json::JsonView& json) const
    {
        AZ::u64 itemCount = 0;
    
        itemCount += SafeGetJSONString(teamInfo.ID, "_id", json);
        itemCount += SafeGetJSONString(teamInfo.Background, "background", json);
        itemCount += SafeGetJSONString(teamInfo.Banner, "banner", json);
        itemCount += SafeGetJSONString(teamInfo.CreatedDate, "created_at", json);
        itemCount += SafeGetJSONString(teamInfo.DisplayName, "display_name", json);
        itemCount += SafeGetJSONString(teamInfo.Info, "info", json);
        itemCount += SafeGetJSONString(teamInfo.Logo, "logo", json);
        itemCount += SafeGetJSONString(teamInfo.Name, "name", json);
        itemCount += SafeGetJSONString(teamInfo.UpdatedDate, "updated_at", json);
    
        return itemCount;
    }

    bool TwitchREST::SafeGetJSONBroadCastType(BroadCastType& type, [[maybe_unused]] const char*key, const Aws::Utils::Json::JsonView& json) const
    {
        bool success = false;
        AZStd::string typeName;

        if( SafeGetJSONString(typeName, "broadcast_type", json) )
        {
            BroadCastType tempType = GetBroadCastTypeFromName(typeName);

            if (tempType != BroadCastType::Default)
            {
                success = true;
                type = tempType;
            }
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoChannel(VideoChannelInfo& channelInfo, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "channel": { "_id": "20694610", "display_name" : "Towelliee", "name" : "towelliee" }
    
        bool success = false;

        if (json.ValueExists("channel"))
        {
            Aws::Utils::Json::JsonView jsonChannel(json.GetObject("channel"));
            
            SafeGetJSONString(channelInfo.ID, "_id", jsonChannel);
            SafeGetJSONString(channelInfo.DisplayName, "display_name", jsonChannel);
            SafeGetJSONString(channelInfo.Name, "name", jsonChannel);
            
            success = true;
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoFPS(FPSInfo & fps, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "fps": { "chunked": 59.9997939597903, "high" : 30.2491085172346, "low" : 30.249192959941, "medium" : 30.2491085172346, "mobile" : 30.249192959941 }

        bool success = false;

        if (json.ValueExists("fps"))
        {
            Aws::Utils::Json::JsonView jsonFPS(json.GetObject("fps"));

            SafeGetJSONdouble(fps.Chunked, "chunked", jsonFPS);
            SafeGetJSONdouble(fps.High, "high", jsonFPS);
            SafeGetJSONdouble(fps.Low, "low", jsonFPS);
            SafeGetJSONdouble(fps.Medium, "medium", jsonFPS);
            SafeGetJSONdouble(fps.Mobile, "mobile", jsonFPS);

            success = true;
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoPreview(PreviewInfo& preview, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "preview": { "large": "https://.../thumb102381501-640x360.jpg","medium" : "https://s...180.jpg","small" : "https://...81501-80x45.jpg", "template" : "https://.../thumb102381501-{width}x{height}.jpg" }
        bool success = false;

        if (json.ValueExists("preview"))
        {
            Aws::Utils::Json::JsonView jsonValue(json.GetObject("preview"));

            SafeGetJSONString(preview.Large, "large", jsonValue);
            SafeGetJSONString(preview.Medium, "medium", jsonValue);
            SafeGetJSONString(preview.Small, "small", jsonValue);
            SafeGetJSONString(preview.Template, "template", jsonValue);

            success = true;
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoResolutions(ResolutionsInfo& resolutions, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "resolutions": {"chunked": "1920x1080","high" : "1280x720","low" : "640x360","medium" : "852x480","mobile" : "400x226"}
        bool success = false;

        if (json.ValueExists("resolutions"))
        {
            Aws::Utils::Json::JsonView jsonValue(json.GetObject("resolutions"));

            SafeGetJSONString(resolutions.Chunked, "chunked", jsonValue);
            SafeGetJSONString(resolutions.High, "high", jsonValue);
            SafeGetJSONString(resolutions.Low, "low", jsonValue);
            SafeGetJSONString(resolutions.Medium, "medium", jsonValue);
            SafeGetJSONString(resolutions.Mobile, "mobile", jsonValue);

            success = true;
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoThumbnailInfo(ThumbnailInfo& info, const char *key, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        // "<key>": [{"type": "generated", "url" : "https://.../thumb102381501-640x360.jpg"}],
        bool success = false;

        if (json.ValueExists(key))
        {
            success = true;
            Aws::Utils::Array<Aws::Utils::Json::JsonView> jsonArray( json.GetArray(key) );

            for (size_t index = 0; index < jsonArray.GetLength(); index++)
            {
                Aws::Utils::Json::JsonView item(jsonArray.GetItem(index));
                
                AZStd::string temp;
                if( SafeGetJSONString(temp, "type", item) )
                {
                    info.Type = temp;
                    temp.clear();
                }

                if (SafeGetJSONString(temp, "url", item))
                {
                    info.Url = temp;
                    temp.clear();
                }
            }
        }

        return success;
    }

    bool TwitchREST::SafeGetJSONVideoThumbnails(ThumbnailsInfo& thumbnails, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        //  "thumbnails": {"large": [{"type": "...", "url" : "..."}],"medium" : [{"type": "...", "url" : "..."}],"small" : [{"type": "...", "url" : "..."}],"template" : [{"type": "...", "url" : "..."}] }
        bool success = false;

        if (json.ValueExists("thumbnails"))
        {
            Aws::Utils::Json::JsonView jsonValue(json.GetObject("thumbnails"));

            SafeGetJSONVideoThumbnailInfo(thumbnails.Large, "large", jsonValue);
            SafeGetJSONVideoThumbnailInfo(thumbnails.Medium, "medium", jsonValue);
            SafeGetJSONVideoThumbnailInfo(thumbnails.Small, "small", jsonValue);
            SafeGetJSONVideoThumbnailInfo(thumbnails.Template, "template", jsonValue);

            success = true;
        }

        return success;
    }

    bool TwitchREST::SafeGetChannelCommunityInfo(CommunityInfo & info, const Aws::Utils::Json::JsonView& json) const
    {
        // assumes the json doc contains
        // { "_id": "", "avatar_image_url": "", "cover_image_url": "", "description": "","description_html": "","language": "", "name": "", "owner_id": "", "rules": "", "rules_html": "", "summary": "" }
    
        AZ::u64 itemCount = 0;

        itemCount += SafeGetJSONString(info.ID, "_id", json);
        itemCount += SafeGetJSONString(info.AvatarImageURL, "avatar_image_url", json);
        itemCount += SafeGetJSONString(info.CoverImageURL, "cover_image_url", json);
        itemCount += SafeGetJSONString(info.Description, "description", json);
        itemCount += SafeGetJSONString(info.DescriptionHTML, "description_html", json);
        itemCount += SafeGetJSONString(info.Language, "language", json);
        itemCount += SafeGetJSONString(info.Name, "name", json);
        itemCount += SafeGetJSONString(info.OwnerID, "owner_id", json);
        itemCount += SafeGetJSONString(info.Rules, "rules", json);
        itemCount += SafeGetJSONString(info.RulesHTML, "rules_html", json);
        itemCount += SafeGetJSONString(info.Summary, "summary", json);

        return (itemCount > 0);
    }

    AZStd::string TwitchREST::GetPresenceAvailabilityName(PresenceAvailability availability) const
    {
        auto itr = m_availabilityMap.find(availability);

        if( itr != m_availabilityMap.end() )
        {
            return itr->second;
        }

        return "";
    }

    AZStd::string TwitchREST::GetPresenceActivityTypeName(PresenceActivityType activityType) const
    {
        auto itr = m_activityTypeMap.find(activityType);

        if (itr != m_activityTypeMap.end())
            return itr->second;

        return "";
    }

    PresenceAvailability TwitchREST::GetPresenceAvailability(const AZStd::string& name) const
    {
        for(const auto& i: m_availabilityMap)
            if( i.second == name)
                return i.first;
    
        return PresenceAvailability::Unknown;
    }

    PresenceActivityType TwitchREST::GetPresenceActivityType(const AZStd::string& name) const
    {
        for (const auto& i : m_activityTypeMap)
            if (i.second == name)
                return i.first;
    
        return PresenceActivityType::Unknown;
    }

    AZStd::string TwitchREST::GetBroadCastTypeNameFromType(BroadCastType type) const
    {
        AZStd::string name;
        AZ::u64 bits = static_cast<AZ::u64>(type);
        
        if( bits & static_cast<AZ::u64>(BroadCastType::Archive) )
        {
            name += "archive";
        }

        if( bits & static_cast<AZ::u64>(BroadCastType::Highlight) )
        {
            if( !name.empty() )
                name += ",";

            name += "highlight";
        }

        if (bits & static_cast<AZ::u64>(BroadCastType::Upload))
        {
            if (!name.empty())
                name += ",";

            name += "upload";
        }
           
        return name;
    }

    BroadCastType TwitchREST::GetBroadCastTypeFromName(const AZStd::string& name) const
    {
        AZ::u64 bits = 0;
    
        if ( name.find("archive") != AZStd::string::npos)
            bits |= static_cast<AZ::u64>(BroadCastType::Archive);

        if (name.find("highlight") != AZStd::string::npos)
            bits |= static_cast<AZ::u64>(BroadCastType::Highlight);

        if (name.find("upload") != AZStd::string::npos)
            bits |= static_cast<AZ::u64>(BroadCastType::Upload);

        return static_cast<BroadCastType>(bits);
    }

    AZ::u64 TwitchREST::GetComercialLength(CommercialLength length) const
    {
        AZ::u64 lengthInSeconds = 0;

        if (length == CommercialLength::T60Seconds)
            lengthInSeconds = 60;
        else if (length == CommercialLength::T90Seconds)
            lengthInSeconds = 90;
        else if (length == CommercialLength::T120Seconds)
            lengthInSeconds = 120;
        else if (length == CommercialLength::T150Seconds)
            lengthInSeconds = 150;
        else if (length == CommercialLength::T180Seconds)
            lengthInSeconds = 180;
        else
            lengthInSeconds = 30; // default is CommercialLength::T30Seconds

        return lengthInSeconds;
    }

}
