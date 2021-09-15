/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Twitch/TwitchBus.h>
#include "TwitchReflection.h"

namespace AZ
{
    /*
    ** Helper macros to reflect the Twitch enums
    */

    AZ_TYPE_INFO_SPECIALIZE(Twitch::ResultCode, "{DA72B2F5-2983-4E30-B64C-BDF417FA73A6}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::PresenceAvailability, "{090CF417-870C-4E27-B9FC-6FE96787DE18}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::PresenceActivityType, "{B8D3EFFC-D71E-4441-9D09-BFD585A4B1B8}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::BroadCastType, "{751DA7A4-A080-4DE4-A15F-F63B2B066AA6}");
    AZ_TYPE_INFO_SPECIALIZE(Twitch::CommercialLength, "{76255136-2B04-4EE2-A499-BBB141A28716}");
}

namespace Twitch
{
    #define ENUMCASE(code) case code: txtCode=AZStd::string::format(#code"(%llu)",static_cast<AZ::u64>(code)); break;

    AZStd::string ResultCodeToString(ResultCode code)
    {
        AZStd::string txtCode;
        switch(code)
        {
            ENUMCASE(ResultCode::Success)
            ENUMCASE(ResultCode::InvalidParam)
            ENUMCASE(ResultCode::TwitchRESTError)
            ENUMCASE(ResultCode::TwitchChannelNoUpdatesToMake)
            ENUMCASE(ResultCode::Unknown)
            default:
                txtCode = AZStd::string::format("Undefined ResultCode (%llu)", static_cast<AZ::u64>(code));
        }

        return txtCode;
    }

    AZStd::string PresenceAvailabilityToString(PresenceAvailability availability)
    {
        AZStd::string txtCode;
        switch (availability)
        {
            ENUMCASE(PresenceAvailability::Unknown)
            ENUMCASE(PresenceAvailability::Online)
            ENUMCASE(PresenceAvailability::Idle)
        default:
            txtCode = AZStd::string::format("Undefined PresenceAvailability (%llu)", static_cast<AZ::u64>(availability));
        }

        return txtCode;
    }

    AZStd::string PresenceActivityTypeToString(PresenceActivityType activity)
    {
        AZStd::string txtCode;
        switch (activity)
        {
            ENUMCASE(PresenceActivityType::Unknown)
            ENUMCASE(PresenceActivityType::Watching)
            ENUMCASE(PresenceActivityType::Playing)
            ENUMCASE(PresenceActivityType::Broadcasting)
        default:
            txtCode = AZStd::string::format("Undefined PresenceActivityType (%llu)", static_cast<AZ::u64>(activity));
        }

        return txtCode;
    }

    AZStd::string BoolName(bool value, const AZStd::string & trueText, const AZStd::string & falseText)
    {
        return value ? trueText : falseText;
    }

    AZStd::string UserNotificationsToString(const UserNotifications & info)
    {
        return "Email: " + BoolName(info.EMail, "On", "Off") +
               "Push: " + BoolName(info.EMail, "On", "Off");
    }

    AZStd::string UserInfoIDToString(const UserInfo & info)
    {
        return  "UserID:" + info.ID;
    }

    AZStd::string UserInfoMiniString(const UserInfo & info)
    {
        return  UserInfoIDToString(info) +
                " DisplayName:" + info.DisplayName +
                " Name:" + info.Name +
                " Type:" + info.Type;
    }

    AZStd::string UserInfoToString(const UserInfo & info)
    {
        return  UserInfoMiniString(info) +
                " Bio:" + info.Bio +
                " EMail:" + info.EMail +
                " Logo:" + info.Logo +
                " Notifications:" + UserNotificationsToString(info.Notifications) +
                " CreatedDate:" + info.CreatedDate +
                " UpdatedDate:" + info.UpdatedDate +
                " EMailVerified:" + BoolName(info.EMailVerified, "Yes", "No") +
                " Partnered:" + BoolName(info.Partnered, "Yes", "No") +
                " TwitterConnected:" + BoolName(info.TwitterConnected, "Yes", "No");
    }

    AZStd::string UserInfoListToString(const UserInfoList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += UserInfoMiniString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string FriendRecommendationToString(const FriendRecommendation & info)
    {
        return  UserInfoIDToString(info.User) +
                " Reason:" + info.Reason;
    }

    AZStd::string FriendRecommendationsToString(const FriendRecommendationList & info)
    {
        AZStd::string strList;

        for (const auto & i : info)
        {
            if (!strList.empty())
            {
                strList += ",";
            }

            strList += "{";
            strList += FriendRecommendationToString(i);
            strList += "}";
        }

        return strList;
    }

    AZStd::string FriendInfoToString(const FriendInfo & info)
   {
       return  UserInfoIDToString(info.User) +
               " CreatedDate:" + info.CreatedDate;
   }

   AZStd::string FriendListToString(const FriendList & info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += FriendInfoToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string FriendRequestToString(const FriendRequest & info)
   {
       return  UserInfoIDToString(info.User) +
               " IsRecommended:" + BoolName(info.IsRecommended, "Yes", "No") +
               " IsStranger:" + BoolName(info.IsStranger, "Yes", "No") +
               " NonStrangerReason:" + info.NonStrangerReason +
               " RequestedDate:" + info.RequestedDate;
   }

   AZStd::string FriendRequestListToString(const FriendRequestList & info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += FriendRequestToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string PresenceStatusToString(const PresenceStatus & info)
   {
       return  "UserID:" + info.UserID +
               " Index:" + AZStd::string::format("%lld", info.Index) +
               " UpdatedDate:" + AZStd::string::format("%lld", info.UpdatedDate) +
               " ActivityType:" + PresenceActivityTypeToString(info.ActivityType) +
               " Availability:" + PresenceAvailabilityToString(info.Availability);
   }

   AZStd::string PresenceStatusListToString(const PresenceStatusList & info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += PresenceStatusToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string PresenceSettingsToString(const PresenceSettings & info)
   {
       return  "IsInvisible:" + BoolName(info.IsInvisible, "Yes", "No") +
               " ShareActivity:" + BoolName(info.ShareActivity, "Shared", "None");
   }

   AZStd::string ChannelInfoToString(const ChannelInfo & info)
   {
       return  "Followers:" + AZStd::string::format("%llu", info.NumFollowers) +
               "Views:" + AZStd::string::format("%llu", info.NumViews) +
               "ItemsRecieved:" + AZStd::string::format("%llu", info.NumItemsRecieved) +
               "Partner:" + BoolName(info.Partner, "Yes", "No") +
               "Mature:" + BoolName(info.Mature, "Yes", "No") +
               "Id:" + info.Id +
               "BroadcasterLanguage:" + info.BroadcasterLanguage +
               "DisplayName:" + info.DisplayName +
               "eMail:" + info.eMail +
               "GameName:" + info.GameName +
               "Language:" + info.Lanugage +
               "Logo:" + info.Logo +
               "Name:" + info.Name +
               "ProfileBanner:" + info.ProfileBanner +
               "ProfileBannerBackgroundColor:" + info.ProfileBannerBackgroundColor +
               "Status:" + info.Status +
               "StreamKey:" + info.StreamKey +
               "UpdatedDate:" + info.UpdatedDate +
               "CreatedDate:" + info.CreatedDate +
               "URL:" + info.URL +
               "VideoBanner:" + info.VideoBanner;
   }

   AZStd::string FollowerToString(const Follower& info)
   {
       return  UserInfoIDToString(info.User) +
               " CreatedDate:" + info.CreatedDate +
               " Notifications:" + BoolName(info.Notifications, "On", "Off");
   }

   AZStd::string FollowerListToString(const FollowerList & info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += FollowerToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string TeamInfoToString(const TeamInfo& info)
   {
       return  "ID:" + info.ID +
               " Background:" + info.Background +
               " Banner:" + info.Banner +
               " CreatedDate:" + info.CreatedDate +
               " DisplayName:" + info.DisplayName +
               " Info:" + info.Info +
               " Logo:" + info.Logo +
               " Name:" + info.Name +
               " UpdatedDate:" + info.UpdatedDate;
   }

   AZStd::string TeamInfoListToString(const TeamInfoList& info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += TeamInfoToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string SubscriberInfoToString(const SubscriberInfo& info)
   {
       return  "ID:" + info.ID +
               " CreatedDate:" + info.CreatedDate +
               UserInfoIDToString(info.User);
   }

   AZStd::string SubscriberInfoListToString(const SubscriberInfoList& info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += SubscriberInfoToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string VideoInfoShortToString(const VideoInfo& info)
   {
       return "ID:" + info.ID;
   }

   AZStd::string VideoInfoListToString(const VideoInfoList& info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += VideoInfoShortToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string StartChannelCommercialResultToString(const StartChannelCommercialResult& info)
   {
       return  "Duration:" + AZStd::string::format("%llu", info.Duration) +
               " RetryAfter:" + AZStd::string::format("%llu", info.RetryAfter) +
               " Message:" + info.Message;
   }

   AZStd::string CommunityInfoToString(const CommunityInfo& info)
   {
       return  "ID:" + info.ID +
               " AvatarImageURL:" + info.AvatarImageURL +
               " CoverImageURL:" + info.CoverImageURL +
               " Description:" + info.Description +
               " DescriptionHTML:" + info.DescriptionHTML +
               " Language:" + info.Language +
               " Name:" + info.Name +
               " OwnerID:" + info.OwnerID +
               " Rules:" + info.Rules +
               " RulesHTML:" + info.RulesHTML +
               " Summary:" + info.Summary;
   }

   AZStd::string CommunityInfoListToString(const CommunityInfoList& info)
   {
       AZStd::string strList;

       for (const auto & i : info)
       {
           if (!strList.empty())
           {
               strList += ",";
           }

           strList += "{";
           strList += CommunityInfoToString(i);
           strList += "}";
       }

       return strList;
   }

   AZStd::string ReturnValueToString(const ReturnValue& info)
   {
       return  "ReceiptID:" + AZStd::string::format("%llu", info.GetID()) +
               " Result: " + ResultCodeToString(info.Result);
   }

   AZStd::string Int64Value::ToString() const
   {
       return  ReturnValueToString(*this) +
               AZStd::string::format(" Int64:%lld", Value);
   }

   AZStd::string Uint64Value::ToString() const
   {
       return  ReturnValueToString(*this) +
               AZStd::string::format(" Uint64:%llu", Value);
   }

   AZStd::string StringValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " String:" + "\"" + Value + "\"";
   }

   AZStd::string UserInfoValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               UserInfoToString(Value);
   }

   AZStd::string FriendRecommendationValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " ListSize:" + AZStd::string::format("%llu-", static_cast<AZ::u64>(Value.size())) +
               " Recommendations:" + FriendRecommendationsToString(Value);
   }

   AZStd::string GetFriendValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " ListSize:" + AZStd::string::format("%llu-", static_cast<AZ::u64>(Value.Friends.size())) +
               " Cursor:" + Value.Cursor +
               " Friends:" + FriendListToString(Value.Friends);
   }

   AZStd::string FriendStatusValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Status:" + Value.Status +
               UserInfoToString(Value.User);
   }

   AZStd::string FriendRequestValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", Value.Total) +
               " Cursor:" + Value.Cursor +
               " Requests:" + FriendRequestListToString(Value.Requests);
   }

   AZStd::string PresenceStatusValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
               " StatusList:" + PresenceStatusListToString(Value);
   }

   AZStd::string PresenceSettingsValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " " + PresenceSettingsToString(Value);
   }

   AZStd::string ChannelInfoValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " " + ChannelInfoToString(Value);
   }

   AZStd::string UserInfoListValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
               " Users:" + UserInfoListToString(Value);
   }

   AZStd::string FollowerResultValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", Value.Total) +
               " Cursor:" + Value.Cursor +
               " Followers:" + FollowerListToString(Value.Followers);
   }

   AZStd::string ChannelTeamValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", static_cast<AZ::u64>(Value.size())) +
               " Teams:" + TeamInfoListToString(Value);
   }

   AZStd::string SubscriberValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", Value.Total) +
               " Subscribers:" + SubscriberInfoListToString(Value.Subscribers);
   }

   AZStd::string SubscriberbyUserValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " SubscriberInfo:" + SubscriberInfoToString(Value);
   }

   AZStd::string VideoReturnValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " Total:" + AZStd::string::format("%llu", Value.Total) +
               " Videos:" + VideoInfoListToString(Value.Videos);
   }

   AZStd::string StartChannelCommercialValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " " + StartChannelCommercialResultToString(Value);
   }

   AZStd::string CommunityInfoValue::ToString() const
   {
       return  ReturnValueToString(*this) +
               " " + CommunityInfoToString(Value);
   }

   AZStd::string CommunityInfoReturnValue::ToString() const
   {
       return  ReturnValueToString(*this) +
           " Total:" + AZStd::string::format("%llu", Value.Total) +
           " Communities:" + CommunityInfoListToString(Value.Communities);
    }

    namespace Internal
    {
        class BehaviorTwitchNotifyBus
            : public TwitchNotifyBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(BehaviorTwitchNotifyBus, "{63EEA49D-1205-4E43-9451-26ACF5771901}", AZ::SystemAllocator,
                UserIDNotify,
                OAuthTokenNotify,
                GetUser,
                ResetFriendsNotificationCountNotify,
                GetFriendNotificationCount,
                GetFriendRecommendations,
                GetFriends,
                GetFriendStatus,
                AcceptFriendRequest,
                GetFriendRequests,
                CreateFriendRequest,
                DeclineFriendRequest,
                UpdatePresenceStatus,
                GetPresenceStatusofFriends,
                GetPresenceSettings,
                UpdatePresenceSettings,
                GetChannel,
                GetChannelbyID,
                UpdateChannel,
                GetChannelEditors,
                GetChannelFollowers,
                GetChannelTeams,
                GetChannelSubscribers,
                CheckChannelSubscriptionbyUser,
                GetChannelVideos,
                StartChannelCommercial,
                ResetChannelStreamKey);

            void UserIDNotify(const StringValue& userID) override
            {
                Call(FN_UserIDNotify, userID);
            }

            void OAuthTokenNotify(const StringValue& token) override
            {
                Call(FN_OAuthTokenNotify, token);
            }

            void GetUser(const UserInfoValue& result) override
            {
                Call(FN_GetUser, result);
            }

            void ResetFriendsNotificationCountNotify(const Int64Value& result) override
            {
                Call(FN_ResetFriendsNotificationCountNotify, result);
            }

            void GetFriendNotificationCount(const Int64Value& result) override
            {
                Call(FN_GetFriendNotificationCount, result);
            }

            void GetFriendRecommendations(const FriendRecommendationValue& result) override
            {
                Call(FN_GetFriendRecommendations, result);
            }

            void GetFriends(const GetFriendValue& result) override
            {
                Call(FN_GetFriends, result);
            }

            void GetFriendStatus(const FriendStatusValue& result) override
            {
                Call(FN_GetFriendStatus, result);
            }

            void AcceptFriendRequest(const Int64Value& result) override
            {
                Call(FN_AcceptFriendRequest, result);
            }

            void GetFriendRequests(const FriendRequestValue& result) override
            {
                Call(FN_GetFriendRequests, result);
            }

            void CreateFriendRequest(const Int64Value& result) override
            {
                Call(FN_CreateFriendRequest, result);
            }

            void DeclineFriendRequest(const Int64Value& result) override
            {
                Call(FN_DeclineFriendRequest, result);
            }

            void UpdatePresenceStatus(const Int64Value& result) override
            {
                Call(FN_UpdatePresenceStatus, result);
            }

            void GetPresenceStatusofFriends(const PresenceStatusValue& result) override
            {
                Call(FN_GetPresenceStatusofFriends, result);
            }

            void GetPresenceSettings(const PresenceSettingsValue& result) override
            {
                Call(FN_GetPresenceSettings, result);
            }

            void UpdatePresenceSettings(const PresenceSettingsValue& result) override
            {
                Call(FN_UpdatePresenceSettings, result);
            }

            void GetChannelbyID(const ChannelInfoValue& result) override
            {
                Call(FN_GetChannelbyID, result);
            }

            void GetChannel(const ChannelInfoValue& result) override
            {
                Call(FN_GetChannel, result);
            }

            void UpdateChannel(const ChannelInfoValue& result) override
            {
                Call(FN_UpdateChannel, result);
            }

            void GetChannelEditors(const UserInfoListValue& result) override
            {
                Call(FN_GetChannelEditors, result);
            }

            void GetChannelFollowers(const FollowerResultValue& result) override
            {
                Call(FN_GetChannelFollowers, result);
            }

            void GetChannelTeams(const ChannelTeamValue& result) override
            {
                Call(FN_GetChannelTeams, result);
            }

            void GetChannelSubscribers(const SubscriberValue& result) override
            {
                Call(FN_GetChannelSubscribers, result);
            }

            void CheckChannelSubscriptionbyUser(const SubscriberbyUserValue& result) override
            {
                Call(FN_CheckChannelSubscriptionbyUser, result);
            }

            void GetChannelVideos(const VideoReturnValue& result) override
            {
                Call(FN_GetChannelVideos, result);
            }

            void StartChannelCommercial(const StartChannelCommercialValue& result) override
            {
                Call(FN_StartChannelCommercial, result);
            }

            void ResetChannelStreamKey(const ChannelInfoValue& result) override
            {
                Call(FN_ResetChannelStreamKey, result);
            }
        };

        /*
        ** helper macro for reflecting the enums
        */

        #define ENUM_CLASS_HELPER(className, enumName) Enum<(int)className::enumName>(#enumName)

        void Reflect(AZ::BehaviorContext & context)
        {
            /*
            ** Reflect the enum's
            */

            context.Class<ResultCode>("ResultCode")->
                ENUM_CLASS_HELPER(ResultCode, Success)->
                ENUM_CLASS_HELPER(ResultCode, InvalidParam)->
                ENUM_CLASS_HELPER(ResultCode, TwitchRESTError)->
                ENUM_CLASS_HELPER(ResultCode, TwitchChannelNoUpdatesToMake)->
                ENUM_CLASS_HELPER(ResultCode, Unknown)
                ;

            context.Class<PresenceAvailability>("PresenceAvailability")->
                ENUM_CLASS_HELPER(PresenceAvailability, Unknown)->
                ENUM_CLASS_HELPER(PresenceAvailability, Online)->
                ENUM_CLASS_HELPER(PresenceAvailability, Idle)
                ;

            context.Class<PresenceActivityType>("PresenceActivityType")->
                ENUM_CLASS_HELPER(PresenceActivityType, Unknown)->
                ENUM_CLASS_HELPER(PresenceActivityType, Watching)->
                ENUM_CLASS_HELPER(PresenceActivityType, Playing)->
                ENUM_CLASS_HELPER(PresenceActivityType, Broadcasting)
                ;

            context.Class<BroadCastType>("BroadCastType")->
                ENUM_CLASS_HELPER(BroadCastType, Default)->
                ENUM_CLASS_HELPER(BroadCastType, Archive)->
                ENUM_CLASS_HELPER(BroadCastType, Highlight)->
                ENUM_CLASS_HELPER(BroadCastType, Upload)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndHighlight)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndUpload)->
                ENUM_CLASS_HELPER(BroadCastType, ArchiveAndHighlightAndUpload)->
                ENUM_CLASS_HELPER(BroadCastType, HighlightAndUpload)
                ;

            context.Class<CommercialLength>("CommercialLength")->
                ENUM_CLASS_HELPER(CommercialLength, T30Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T60Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T90Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T120Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T150Seconds)->
                ENUM_CLASS_HELPER(CommercialLength, T180Seconds)
                ;

            context.Class<ReceiptID>()->
                Method("Equal", &ReceiptID::operator==)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
                Property("ID", &ReceiptID::GetID, &ReceiptID::SetID)
                ;

            context.Class<Int64Value>()->
                Property("Value", [](const Int64Value& i64Value) { return i64Value.Value; }, nullptr)->
                Property("Result", [](const Int64Value& i64Value) { return i64Value.Result; }, nullptr)->
                Method("ToString", &Int64Value::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<Uint64Value>()->
                Property("Value", [](const Uint64Value& u64Value) { return u64Value.Value; }, nullptr)->
                Property("Result", [](const Uint64Value& u64Value) { return u64Value.Result; }, nullptr)->
                Method("ToString", &Uint64Value::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<StringValue>()->
                Property("Value", [](const StringValue& strValue) { return strValue.Value; }, nullptr)->
                Property("Result", [](const StringValue& strValue) { return strValue.Result; }, nullptr)->
                Method("ToString", &StringValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<UserNotifications>()->
                Property("EMail", [](const UserNotifications& value) { return value.EMail; }, nullptr)->
                Property("Push", [](const UserNotifications& value) { return value.Push; }, nullptr)
                ;

            context.Class<UserInfo>()->
                Property("ID", [](const UserInfo& value) { return value.ID; }, nullptr)->
                Property("Bio", [](const UserInfo& value) { return value.Bio; }, nullptr)->
                Property("CreatedDate", [](const UserInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("DisplayName", [](const UserInfo& value) { return value.DisplayName; }, nullptr)->
                Property("EMail", [](const UserInfo& value) { return value.EMail; }, nullptr)->
                Property("Logo", [](const UserInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const UserInfo& value) { return value.Name; }, nullptr)->
                Property("ProfileBanner", [](const UserInfo& value) { return value.ProfileBanner; }, nullptr)->
                Property("ProfileBannerBackgroundColor", [](const UserInfo& value) { return value.ProfileBannerBackgroundColor; }, nullptr)->
                Property("Type", [](const UserInfo& value) { return value.Type; }, nullptr)->
                Property("UpdatedDate", [](const UserInfo& value) { return value.UpdatedDate; }, nullptr)->
                Property("Notifications", [](const UserInfo& value) { return value.Notifications; }, nullptr)->
                Property("EMailVerified", [](const UserInfo& value) { return value.EMailVerified; }, nullptr)->
                Property("Partnered", [](const UserInfo& value) { return value.Partnered; }, nullptr)->
                Property("TwitterConnected", [](const UserInfo& value) { return value.TwitterConnected; }, nullptr)
                ;

            context.Class<UserInfoValue>()->
                Property("Value", [](const UserInfoValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const UserInfoValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &UserInfoValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<FriendRecommendation>()->
                Property("Reason", [](const FriendRecommendation& value) { return value.Reason; }, nullptr)->
                Property("User", [](const FriendRecommendation& value) { return value.User; }, nullptr)
                ;

            context.Class<FriendRecommendationValue>()->
                Property("Value", [](const FriendRecommendationValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const FriendRecommendationValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &FriendRecommendationValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<GetFriendReturn>()->
                Property("Cursor", [](const GetFriendReturn& value) { return value.Cursor; }, nullptr)->
                Property("Friends", [](const GetFriendReturn& value) { return value.Friends; }, nullptr)
                ;

            context.Class<GetFriendValue>()->
                Property("Value", [](const GetFriendValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const GetFriendValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &GetFriendValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<FriendStatus>()->
                Property("Status", [](const FriendStatus& value) { return value.Status; }, nullptr)->
                Property("User", [](const FriendStatus& value) { return value.User; }, nullptr)
                ;

            context.Class<FriendStatusValue>()->
                Property("Value", [](const FriendStatusValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const FriendStatusValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &FriendStatusValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<FriendRequest>()->
                Property("IsRecommended", [](const FriendRequest& value) { return value.IsRecommended; }, nullptr)->
                Property("IsStranger", [](const FriendRequest& value) { return value.IsStranger; }, nullptr)->
                Property("NonStrangerReason", [](const FriendRequest& value) { return value.NonStrangerReason; }, nullptr)->
                Property("RequestedDate", [](const FriendRequest& value) { return value.RequestedDate; }, nullptr)->
                Property("User", [](const FriendRequest& value) { return value.User; }, nullptr)
                ;

            context.Class<FriendRequestResult>()->
                Property("Total", [](const FriendRequestResult& value) { return value.Total; }, nullptr)->
                Property("Cursor", [](const FriendRequestResult& value) { return value.Cursor; }, nullptr)->
                Property("Requests", [](const FriendRequestResult& value) { return value.Requests; }, nullptr)
                ;

            context.Class<FriendRequestValue>()->
                Property("Value", [](const FriendRequestValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const FriendRequestValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &FriendRequestValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<PresenceStatus>()->
                Property("ActivityType", [](const PresenceStatus& value) { return value.ActivityType; }, nullptr)->
                Property("Availability", [](const PresenceStatus& value) { return value.Availability; }, nullptr)->
                Property("Index", [](const PresenceStatus& value) { return value.Index; }, nullptr)->
                Property("UpdatedDate", [](const PresenceStatus& value) { return value.UpdatedDate; }, nullptr)->
                Property("UserID", [](const PresenceStatus& value) { return value.UserID; }, nullptr)
                ;

            context.Class<PresenceStatusValue>()->
                Property("Value", [](const PresenceStatusValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const PresenceStatusValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &PresenceStatusValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<PresenceSettings>()->
                Property("IsInvisible", [](const PresenceSettings& value) { return value.IsInvisible; }, nullptr)->
                Property("ShareActivity", [](const PresenceSettings& value) { return value.ShareActivity; }, nullptr)
                ;

            context.Class<PresenceSettingsValue>()->
                Property("Value", [](const PresenceSettingsValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const PresenceSettingsValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &PresenceSettingsValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<ChannelInfo>()->
                Property("NumFollowers", [](const ChannelInfo& value) { return value.NumFollowers; }, nullptr)->
                Property("NumViews", [](const ChannelInfo& value) { return value.NumViews; }, nullptr)->
                Property("NumItemsRecieved", [](const ChannelInfo& value) { return value.NumItemsRecieved; }, nullptr)->
                Property("Partner", [](const ChannelInfo& value) { return value.Partner; }, nullptr)->
                Property("Mature", [](const ChannelInfo& value) { return value.Mature; }, nullptr)->
                Property("Id", [](const ChannelInfo& value) { return value.Id; }, nullptr)->
                Property("BroadcasterLanguage", [](const ChannelInfo& value) { return value.BroadcasterLanguage; }, nullptr)->
                Property("DisplayName", [](const ChannelInfo& value) { return value.DisplayName; }, nullptr)->
                Property("eMail", [](const ChannelInfo& value) { return value.eMail; }, nullptr)->
                Property("GameName", [](const ChannelInfo& value) { return value.GameName; }, nullptr)->
                Property("Lanugage", [](const ChannelInfo& value) { return value.Lanugage; }, nullptr)->
                Property("Logo", [](const ChannelInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const ChannelInfo& value) { return value.Name; }, nullptr)->
                Property("ProfileBanner", [](const ChannelInfo& value) { return value.ProfileBanner; }, nullptr)->
                Property("ProfileBannerBackgroundColor", [](const ChannelInfo& value) { return value.ProfileBannerBackgroundColor; }, nullptr)->
                Property("Status", [](const ChannelInfo& value) { return value.Status; }, nullptr)->
                Property("StreamKey", [](const ChannelInfo& value) { return value.StreamKey; }, nullptr)->
                Property("UpdatedDate", [](const ChannelInfo& value) { return value.UpdatedDate; }, nullptr)->
                Property("CreatedDate", [](const ChannelInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("URL", [](const ChannelInfo& value) { return value.URL; }, nullptr)->
                Property("VideoBanner", [](const ChannelInfo& value) { return value.VideoBanner; }, nullptr)
                ;

            context.Class<ChannelInfoValue>()->
                Property("Value", [](const ChannelInfoValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const ChannelInfoValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &ChannelInfoValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<UpdateValuebool>()->
                Property("Value", &UpdateValuebool::GetValue, &UpdateValuebool::SetValue)->
                Method("ToBeUpdated", &UpdateValuebool::ToBeUpdated)
                ;

            context.Class<UpdateValueuint>()->
                Property("Value", &UpdateValueuint::GetValue, &UpdateValueuint::SetValue)->
                Method("ToBeUpdated", &UpdateValueuint::ToBeUpdated)
                ;

            context.Class<UpdateValuestring>()->
                Property("Value", &UpdateValuestring::GetValue, &UpdateValuestring::SetValue)->
                Method("ToBeUpdated", &UpdateValuestring::ToBeUpdated)
                ;

            context.Class<ChannelUpdateInfo>()->
                Property("ChannelFeedEnabled", BehaviorValueProperty(&ChannelUpdateInfo::ChannelFeedEnabled))->
                Property("Delay", BehaviorValueProperty(&ChannelUpdateInfo::Delay))->
                Property("Status", BehaviorValueProperty(&ChannelUpdateInfo::Status))->
                Property("GameName", BehaviorValueProperty(&ChannelUpdateInfo::GameName))
                ;

            context.Class<UserInfoListValue>()->
                Property("Value", [](const UserInfoListValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const UserInfoListValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &UserInfoListValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<Follower>()->
                Property("Notifications", [](const Follower& value) { return value.Notifications; }, nullptr)->
                Property("CreatedDate", [](const Follower& value) { return value.CreatedDate; }, nullptr)->
                Property("User", [](const Follower& value) { return value.User; }, nullptr)
                ;

            context.Class<FollowerResult>()->
                Property("Total", [](const FollowerResult& value) { return value.Total; }, nullptr)->
                Property("Cursor", [](const FollowerResult& value) { return value.Cursor; }, nullptr)->
                Property("Followers", [](const FollowerResult& value) { return value.Followers; }, nullptr)
                ;

            context.Class<FollowerResultValue>()->
                Property("Value", [](const FollowerResultValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const FollowerResultValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &FollowerResultValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<TeamInfo>()->
                Property("ID", [](const TeamInfo& value) { return value.ID; }, nullptr)->
                Property("Background", [](const TeamInfo& value) { return value.Background; }, nullptr)->
                Property("Banner", [](const TeamInfo& value) { return value.Banner; }, nullptr)->
                Property("CreatedDate", [](const TeamInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("DisplayName", [](const TeamInfo& value) { return value.DisplayName; }, nullptr)->
                Property("Info", [](const TeamInfo& value) { return value.Info; }, nullptr)->
                Property("Logo", [](const TeamInfo& value) { return value.Logo; }, nullptr)->
                Property("Name", [](const TeamInfo& value) { return value.Name; }, nullptr)->
                Property("UpdatedDate", [](const TeamInfo& value) { return value.UpdatedDate; }, nullptr)
                ;

            context.Class<ChannelTeamValue>()->
                Property("Value", [](const ChannelTeamValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const ChannelTeamValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &ChannelTeamValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<SubscriberInfo>()->
                Property("ID", [](const SubscriberInfo& value) { return value.ID; }, nullptr)->
                Property("CreatedDate", [](const SubscriberInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("User", [](const SubscriberInfo& value) { return value.User; }, nullptr)
                ;

            context.Class<Subscription>()->
                Property("Total", [](const Subscription& value) { return value.Total; }, nullptr)->
                Property("Subscribers", [](const Subscription& value) { return value.Subscribers; }, nullptr)
                ;

            context.Class<SubscriberValue>()->
                Property("Value", [](const SubscriberValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const SubscriberValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &SubscriberValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<SubscriberbyUserValue>()->
                Property("Value", [](const SubscriberbyUserValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const SubscriberbyUserValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &SubscriberbyUserValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<VideoChannelInfo>()->
                Property("ID", [](const VideoChannelInfo& value) { return value.ID; }, nullptr)->
                Property("DisplayName", [](const VideoChannelInfo& value) { return value.DisplayName; }, nullptr)->
                Property("Name", [](const VideoChannelInfo& value) { return value.Name; }, nullptr)
                ;

            context.Class<FPSInfo>()->
                Property("Chunked", [](const FPSInfo& value) { return value.Chunked; }, nullptr)->
                Property("High", [](const FPSInfo& value) { return value.High; }, nullptr)->
                Property("Low", [](const FPSInfo& value) { return value.Low; }, nullptr)->
                Property("Medium", [](const FPSInfo& value) { return value.Medium; }, nullptr)->
                Property("Mobile", [](const FPSInfo& value) { return value.Mobile; }, nullptr)
                ;

            context.Class<PreviewInfo>()->
                Property("Large", [](const PreviewInfo& value) { return value.Large; }, nullptr)->
                Property("Medium", [](const PreviewInfo& value) { return value.Medium; }, nullptr)->
                Property("Small", [](const PreviewInfo& value) { return value.Small; }, nullptr)->
                Property("Template", [](const PreviewInfo& value) { return value.Template; }, nullptr)
                ;

            context.Class<ResolutionsInfo>()->
                Property("Chunked", [](const ResolutionsInfo& value) { return value.Chunked; }, nullptr)->
                Property("High", [](const ResolutionsInfo& value) { return value.High; }, nullptr)->
                Property("Low", [](const ResolutionsInfo& value) { return value.Low; }, nullptr)->
                Property("Medium", [](const ResolutionsInfo& value) { return value.Medium; }, nullptr)->
                Property("Mobile", [](const ResolutionsInfo& value) { return value.Mobile; }, nullptr)
                ;

            context.Class<ThumbnailInfo>()->
                Property("Type", [](const ThumbnailInfo& value) { return value.Type; }, nullptr)->
                Property("Url", [](const ThumbnailInfo& value) { return value.Url; }, nullptr)
                ;

            context.Class<ThumbnailsInfo>()->
                Property("Large", [](const ThumbnailsInfo& value) { return value.Large; }, nullptr)->
                Property("Medium", [](const ThumbnailsInfo& value) { return value.Medium; }, nullptr)->
                Property("Small", [](const ThumbnailsInfo& value) { return value.Small; }, nullptr)->
                Property("Template", [](const ThumbnailsInfo& value) { return value.Template; }, nullptr)
                ;

            context.Class<VideoInfo>()->
                Property("Length", [](const VideoInfo& value) { return value.Length; }, nullptr)->
                Property("Views", [](const VideoInfo& value) { return value.Views; }, nullptr)->
                Property("BroadcastID", [](const VideoInfo& value) { return value.BroadcastID; }, nullptr)->
                Property("Type", [](const VideoInfo& value) { return value.Type; }, nullptr)->
                Property("CreatedDate", [](const VideoInfo& value) { return value.CreatedDate; }, nullptr)->
                Property("Description", [](const VideoInfo& value) { return value.Description; }, nullptr)->
                Property("DescriptionHTML", [](const VideoInfo& value) { return value.DescriptionHTML; }, nullptr)->
                Property("ID", [](const VideoInfo& value) { return value.ID; }, nullptr)->
                Property("Game", [](const VideoInfo& value) { return value.Game; }, nullptr)->
                Property("Language", [](const VideoInfo& value) { return value.Language; }, nullptr)->
                Property("PublishedDate", [](const VideoInfo& value) { return value.PublishedDate; }, nullptr)->
                Property("Status", [](const VideoInfo& value) { return value.Status; }, nullptr)->
                Property("TagList", [](const VideoInfo& value) { return value.TagList; }, nullptr)->
                Property("Title", [](const VideoInfo& value) { return value.Title; }, nullptr)->
                Property("URL", [](const VideoInfo& value) { return value.URL; }, nullptr)->
                Property("Viewable", [](const VideoInfo& value) { return value.Viewable; }, nullptr)->
                Property("ViewableAt", [](const VideoInfo& value) { return value.ViewableAt; }, nullptr)->
                Property("Channel", [](const VideoInfo& value) { return value.Channel; }, nullptr)->
                Property("FPS", [](const VideoInfo& value) { return value.FPS; }, nullptr)->
                Property("Preview", [](const VideoInfo& value) { return value.Preview; }, nullptr)->
                Property("Thumbnails", [](const VideoInfo& value) { return value.Thumbnails; }, nullptr)->
                Property("Resolutions", [](const VideoInfo& value) { return value.Resolutions; }, nullptr)
                ;

            context.Class<VideoReturn>()->
                Property("Total", [](const VideoReturn& value) { return value.Total; }, nullptr)->
                Property("Videos", [](const VideoReturn& value) { return value.Videos; }, nullptr)
                ;

            context.Class<VideoReturnValue>()->
                Property("Value", [](const VideoReturnValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const VideoReturnValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &VideoReturnValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<StartChannelCommercialResult>()->
                Property("Duration", [](const StartChannelCommercialResult& value) { return value.Duration; }, nullptr)->
                Property("RetryAfter", [](const StartChannelCommercialResult& value) { return value.RetryAfter; }, nullptr)->
                Property("Message", [](const StartChannelCommercialResult& value) { return value.Message; }, nullptr)
                ;

            context.Class<StartChannelCommercialValue>()->
                Property("Value", [](const StartChannelCommercialValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const StartChannelCommercialValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &StartChannelCommercialValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<CommunityInfo>()->
                Property("ID", [](const CommunityInfo& value) { return value.ID; }, nullptr)->
                Property("AvatarImageURL", [](const CommunityInfo& value) { return value.AvatarImageURL; }, nullptr)->
                Property("CoverImageURL", [](const CommunityInfo& value) { return value.CoverImageURL; }, nullptr)->
                Property("Description", [](const CommunityInfo& value) { return value.Description; }, nullptr)->
                Property("DescriptionHTML", [](const CommunityInfo& value) { return value.DescriptionHTML; }, nullptr)->
                Property("Language", [](const CommunityInfo& value) { return value.Language; }, nullptr)->
                Property("Name", [](const CommunityInfo& value) { return value.Name; }, nullptr)->
                Property("OwnerID", [](const CommunityInfo& value) { return value.OwnerID; }, nullptr)->
                Property("Rules", [](const CommunityInfo& value) { return value.Rules; }, nullptr)->
                Property("RulesHTML", [](const CommunityInfo& value) { return value.RulesHTML; }, nullptr)->
                Property("Summary", [](const CommunityInfo& value) { return value.Summary; }, nullptr)
                ;

            context.Class<CommunityInfoValue>()->
                Property("Value", [](const CommunityInfoValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const CommunityInfoValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &CommunityInfoValue::ToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.Class<CommunityInfoReturn>()->
                Property("Total", [](const CommunityInfoReturn& value) { return value.Total; }, nullptr)->
                Property("Communities", [](const CommunityInfoReturn& value) { return value.Communities; }, nullptr)
                ;

            context.Class<CommunityInfoReturnValue>()->
                Property("Value", [](const CommunityInfoReturnValue& value) { return value.Value; }, nullptr)->
                Property("Result", [](const CommunityInfoReturnValue& value) { return value.Result; }, nullptr)->
                Method("ToString", &CommunityInfoReturnValue::ToString)->
                Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ;

            context.EBus<TwitchRequestBus>("TwitchRequestBus")
                ->Event("SetApplicationID", &TwitchRequestBus::Events::SetApplicationID)
                ->Event("GetApplicationID", &TwitchRequestBus::Events::GetApplicationID)
                ->Event("GetUserID", &TwitchRequestBus::Events::GetUserID)
                ->Event("GetOAuthToken", &TwitchRequestBus::Events::GetOAuthToken)
                ->Event("GetSessionID", &TwitchRequestBus::Events::GetSessionID)
                ->Event("SetUserID", &TwitchRequestBus::Events::SetUserID)
                ->Event("SetOAuthToken", &TwitchRequestBus::Events::SetOAuthToken)
                ->Event("RequestUserID", &TwitchRequestBus::Events::RequestUserID)
                ->Event("RequestOAuthToken", &TwitchRequestBus::Events::RequestOAuthToken)
                ->Event("GetUser", &TwitchRequestBus::Events::GetUser)
                ->Event("ResetFriendsNotificationCount", &TwitchRequestBus::Events::ResetFriendsNotificationCount)
                ->Event("GetFriendNotificationCount", &TwitchRequestBus::Events::GetFriendNotificationCount)
                ->Event("GetFriendRecommendations", &TwitchRequestBus::Events::GetFriendRecommendations)
                ->Event("GetFriends", &TwitchRequestBus::Events::GetFriends)
                ->Event("GetFriendStatus", &TwitchRequestBus::Events::GetFriendStatus)
                ->Event("AcceptFriendRequest", &TwitchRequestBus::Events::AcceptFriendRequest)
                ->Event("GetFriendRequests", &TwitchRequestBus::Events::GetFriendRequests)
                ->Event("CreateFriendRequest", &TwitchRequestBus::Events::CreateFriendRequest)
                ->Event("DeclineFriendRequest", &TwitchRequestBus::Events::DeclineFriendRequest)
                ->Event("UpdatePresenceStatus", &TwitchRequestBus::Events::UpdatePresenceStatus)
                ->Event("GetPresenceStatusofFriends", &TwitchRequestBus::Events::GetPresenceStatusofFriends)
                ->Event("GetPresenceSettings", &TwitchRequestBus::Events::GetPresenceSettings)
                ->Event("UpdatePresenceSettings", &TwitchRequestBus::Events::UpdatePresenceSettings)
                ->Event("GetChannel", &TwitchRequestBus::Events::GetChannel)
                ->Event("GetChannelbyID", &TwitchRequestBus::Events::GetChannelbyID)
                ->Event("UpdateChannel", &TwitchRequestBus::Events::UpdateChannel)
                ->Event("GetChannelEditors", &TwitchRequestBus::Events::GetChannelEditors)
                ->Event("GetChannelFollowers", &TwitchRequestBus::Events::GetChannelFollowers)
                ->Event("GetChannelTeams", &TwitchRequestBus::Events::GetChannelTeams)
                ->Event("GetChannelSubscribers", &TwitchRequestBus::Events::GetChannelSubscribers)
                ->Event("CheckChannelSubscriptionbyUser", &TwitchRequestBus::Events::CheckChannelSubscriptionbyUser)
                ->Event("GetChannelVideos", &TwitchRequestBus::Events::GetChannelVideos)
                ->Event("StartChannelCommercial", &TwitchRequestBus::Events::StartChannelCommercial)
                ->Event("ResetChannelStreamKey", &TwitchRequestBus::Events::ResetChannelStreamKey)
                ;

            context.EBus<TwitchNotifyBus>("TwitchNotifyBus")
                ->Handler<BehaviorTwitchNotifyBus>();
        }
    }
}
