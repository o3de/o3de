/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "BaseTypes.h"

namespace Twitch
{
    /*
    ** Twitch user info data.
    **  Note: Not all of the API's will fill in all of the members.
    */

    struct UserNotifications
    {
        AZ_TYPE_INFO(UserNotifications, "{D6E3A0C4-2E0C-45B5-A8A6-9F9441F9C725}");

        UserNotifications()
            : EMail(false)
            , Push(false)
        {}

        bool EMail;
        bool Push;
    };


    struct UserInfo
    {
        AZ_TYPE_INFO(UserInfo, "{35ACEAC3-A197-4403-9452-443D66BDF9FC}");

        UserInfo() 
            : EMailVerified(false)
            , Partnered(false)
            , TwitterConnected(false)
        {}

        AZStd::string       ID;
        AZStd::string       Bio;
        AZStd::string       CreatedDate;
        AZStd::string       DisplayName;
        AZStd::string       EMail;
        AZStd::string       Logo;
        AZStd::string       Name;
        AZStd::string       ProfileBanner;
        AZStd::string       ProfileBannerBackgroundColor;
        AZStd::string       Type;
        AZStd::string       UpdatedDate;
        UserNotifications   Notifications;
        bool                EMailVerified;
        bool                Partnered;
        bool                TwitterConnected;
    };

    CreateReturnTypeClass(UserInfoValue, UserInfo, "{9631F3C4-48F5-4CA2-AB18-418D58A903E8}");

    /*
    ** return type for GetFriendsRecommendations
    */

    struct FriendRecommendation
    {
        AZ_TYPE_INFO(FriendRecommendation, "{0E1EAA3A-0117-40DA-9444-24B82A03454F}");
        AZStd::string       Reason;
        UserInfo            User;
    };

    using FriendRecommendationList = AZStd::list<FriendRecommendation>;

    CreateReturnTypeClass(FriendRecommendationValue, FriendRecommendationList, "{30F99CBA-CE66-4A7B-B83E-6DEC045128BA}");

    /*
    ** return type for GetFriends
    */
    
    struct FriendInfo
    {
        AZ_TYPE_INFO(FriendInfo, "{EFCC6A81-CE3A-4A30-81DE-9FF283C8820C}");
        AZStd::string        CreatedDate;
        UserInfo             User;
    };
    
    using FriendList = AZStd::list<FriendInfo>;

    struct GetFriendReturn
    {
        AZ_TYPE_INFO(GetFriendReturn, "{B7EE23A0-82D5-4D1A-ADB1-DC2C8B6C3AB9}");
        AZStd::string       Cursor;
        FriendList          Friends;
    };

    CreateReturnTypeClass(GetFriendValue, GetFriendReturn, "{1438E4BA-5D6C-475D-B6A0-A91F1669E215}");

    /*
    ** return type for GetFriendsStatus
    */

    struct FriendStatus
    {
        AZ_TYPE_INFO(FriendStatus, "{03C65FBC-4DB8-4B1E-9B71-94749DDADBFB}");
        AZStd::string       Status;
        UserInfo            User;
    };

    CreateReturnTypeClass(FriendStatusValue, FriendStatus, "{13594A2E-A5B2-44CD-AD31-66154B3E94E7}");

    /*
    ** return type for GetOpenFriendsRequests
    */

    struct FriendRequest
    {
        AZ_TYPE_INFO(FriendRequest, "{A95CF0FA-1B0A-42EB-8F01-1E9AD61CB6CB}");
        FriendRequest() : IsRecommended(false), IsStranger(false) {}

        bool            IsRecommended;
        bool            IsStranger;
        AZStd::string   NonStrangerReason;
        AZStd::string   RequestedDate;
        UserInfo        User;
    };

    using FriendRequestList = AZStd::list<FriendRequest>;

    struct FriendRequestResult
    {
        AZ_TYPE_INFO(FriendRequestResult, "{96F2F93F-AFC6-4EAA-BCF1-BFEA27B979E0}");
        AZ::u64             Total;
        AZStd::string       Cursor;
        FriendRequestList   Requests;
    };

    CreateReturnTypeClass(FriendRequestValue, FriendRequestResult, "{A0E68A16-EC18-48EF-95A5-E3F17D80AB34}");

    /*
    ** Used in UpddatePresenceStatus
    */

    enum class PresenceAvailability { Unknown, Online, Idle };
    enum class PresenceActivityType { Unknown, Watching, Playing, Broadcasting};

    /*
    ** Return type for GetPresenceStatusOfFriends
    */

    struct PresenceStatus
    {
        AZ_TYPE_INFO(PresenceStatus, "{481F5471-C70F-4716-B079-B32BA1FA24FA}");
        PresenceStatus()
            : ActivityType(PresenceActivityType::Unknown)
            , Availability(PresenceAvailability::Unknown)
            , Index(0)
            , UpdatedDate(0)
        {}

        PresenceActivityType    ActivityType;
        PresenceAvailability    Availability;
        AZ::s64                 Index;
        AZ::s64                 UpdatedDate;
        AZStd::string           UserID;
    };

    using PresenceStatusList = AZStd::list<PresenceStatus>;

    CreateReturnTypeClass(PresenceStatusValue, PresenceStatusList, "{3A41EBA7-8416-4A5E-8465-06BBC720C76B}");

    /*
    ** Return type for GetPresenceSettings and UpdatePresenceSettings
    */

    struct PresenceSettings
    {
        AZ_TYPE_INFO(PresenceSettings, "{BAD33E02-B48F-4807-B26E-D45B10D12497}");
        PresenceSettings()
            : IsInvisible(false)
            , ShareActivity(false)
        {}

        bool            IsInvisible;
        bool            ShareActivity;
    };

    CreateReturnTypeClass(PresenceSettingsValue, PresenceSettings, "{02DF54C3-4504-4341-B890-D6E800F012A2}");

    /*
    ** Return info for GetChannel, GetChannelbyID, UpdateChannel and ResetChannelStreamKey methods
    */

    struct ChannelInfo
    {
        AZ_TYPE_INFO(ChannelInfo, "{CA7F7F91-D88C-43E6-B7A8-E16847953070}");
        ChannelInfo() 
            : NumFollowers(0)
            , NumViews(0)
            , NumItemsRecieved(0)
            , Partner(false)
            , Mature(false)
        {}

        AZ::u64                 NumFollowers;
        AZ::u64                 NumViews;
        AZ::u64                 NumItemsRecieved;
        bool                    Partner;
        bool                    Mature;
        AZStd::string           Id;
        AZStd::string           BroadcasterLanguage;
        AZStd::string           DisplayName;
        AZStd::string           eMail;
        AZStd::string           GameName;
        AZStd::string           Lanugage;
        AZStd::string           Logo;
        AZStd::string           Name;
        AZStd::string           ProfileBanner;
        AZStd::string           ProfileBannerBackgroundColor;
        AZStd::string           Status;
        AZStd::string           StreamKey;
        AZStd::string           UpdatedDate;
        AZStd::string           CreatedDate;
        AZStd::string           URL;
        AZStd::string           VideoBanner;
    };

    CreateReturnTypeClass(ChannelInfoValue, ChannelInfo, "{4A8F57CC-B202-4F20-9340-87291134F588}");

    /*
    ** UpdateChanelinfo - Updates specified properties of a specified channel.
    **   Note: Only call SetValue() on the item that you want to update, at least one
    **   of the items must be set to update the channel info or the call to UpdateChannelInfo
    **   will fail with a result code of ResultCode::TwitchChannelNoUpdatesToMake
    **     
    */

    #define DEFINE_UPDATE_TYPE(updateName, updateType, classGUID)                               \
        class updateName                                                                        \
        {                                                                                       \
        public:                                                                                 \
            AZ_TYPE_INFO(updateName, classGUID);                                                \
            updateName() : IsUpdated(false) {}                                                  \
            virtual ~updateName() {}                                                            \
            void SetValue(const updateType& v) { IsUpdated = true; Value = v; }                 \
            updateType GetValue() const { return Value; }                                       \
            bool ToBeUpdated() const { return IsUpdated; }                                      \
        private:                                                                                \
            updateType      Value;                                                              \
            bool            IsUpdated;                                                          \
        }

    DEFINE_UPDATE_TYPE(UpdateValuebool, bool, "{27A2FF93-1BDC-4C0C-971F-577D8F0A941F}");
    DEFINE_UPDATE_TYPE(UpdateValueuint, AZ::u64, "{47B974F8-D1FB-4746-9DC1-90928747B07F}");
    DEFINE_UPDATE_TYPE(UpdateValuestring, AZStd::string, "{CE0A3C4C-596F-49E2-A3A8-897B6B39652D}");
        
    struct ChannelUpdateInfo
    {
        AZ_TYPE_INFO(ChannelUpdateInfo, "{2F8547C5-C915-4DCB-ABBA-1855CEF51B7C}");

        UpdateValuebool              ChannelFeedEnabled;     // If true, the channels feed is turned on. Default: false.
        UpdateValueuint              Delay;                  // Channel delay, in seconds. This inserts a delay in the live feed.
        UpdateValuestring            Status;                 // Description of the broadcasters status, displayed as a title on the channel page.
        UpdateValuestring            GameName;               // Name of game
    };

    /*
    ** GetChannelEditors - Gets a list of users who are editors for a specified channel.
    **  Note: UserInfo ProfileBanner and ProfileBannerBackgroundColor members are
    **        not used in the UserInfo struct and will be empty.
    */

    using UserInfoList = AZStd::list<UserInfo>;

    CreateReturnTypeClass(UserInfoListValue, UserInfoList, "{35A0859C-3C2A-4BE7-B00A-E202D0B20868}");

    /*
    ** GetChannelFollowers - Gets a list of users who follow a specified channel, sorted by the date when
    **                       they started following the channel (newest first, unless specified otherwise).
    **
    **  Note: UserInfo ProfileBanner and ProfileBannerBackgroundColor members are
    **        not used in the UserInfo struct and will be empty.
    */

    struct Follower
    {
        AZ_TYPE_INFO(Follower, "{178A6465-B476-404F-86DF-40BC7167A41F}");

        Follower() : Notifications(false) {}

        bool            Notifications;
        AZStd::string   CreatedDate;
        UserInfo        User;
    };

    using FollowerList = AZStd::list<Follower>;

    struct FollowerResult
    {
        AZ_TYPE_INFO(FollowerResult, "{D05365AE-B483-4BF6-8DBA-3960714BBEA6}");

        FollowerResult() : Total(0) {}

        AZ::u64             Total;
        AZStd::string       Cursor;
        FollowerList        Followers;
    };

    CreateReturnTypeClass(FollowerResultValue, FollowerResult, "{0866A31F-5622-4694-8572-0DA400F4FEAB}");

    /*
    ** GetChannelTeams - Gets a list of teams to which a specified channel belongs.
    */

    struct TeamInfo
    {
        AZ_TYPE_INFO(TeamInfo, "{8378F30A-7809-4116-9F67-D5E551EFABC7}");

        AZStd::string       ID;
        AZStd::string       Background;
        AZStd::string       Banner;
        AZStd::string       CreatedDate;
        AZStd::string       DisplayName;
        AZStd::string       Info;
        AZStd::string       Logo;
        AZStd::string       Name;
        AZStd::string       UpdatedDate;
    };

    using TeamInfoList = AZStd::list<TeamInfo>;

    CreateReturnTypeClass(ChannelTeamValue, TeamInfoList, "{294606E1-F822-4F06-AE74-7450BE41E7F7}");

    /*
    ** GetChannelSubscribers - Gets a list of users subscribed to a specified channel, sorted by the date when they subscribed.
    **
    **  Note: UserInfo ProfileBanner and ProfileBannerBackgroundColor members are
    **        not used in the UserInfo struct and will be empty.
    */

    struct SubscriberInfo
    {
        AZ_TYPE_INFO(SubscriberInfo, "{93662C6B-A871-4213-934D-1475C2EE0C8F}");

        AZStd::string       ID;
        AZStd::string       CreatedDate;
        UserInfo            User;
    };

    using SubscriberInfoList = AZStd::list<SubscriberInfo>;

    struct Subscription
    {
        AZ_TYPE_INFO(Subscription, "{0C11FBFF-BAA2-4668-9192-69FA67452382}");

        AZ::u64             Total;
        SubscriberInfoList  Subscribers;
    };

    CreateReturnTypeClass(SubscriberValue, Subscription, "{1F055008-0944-4CA6-B4E3-2BD8909666E3}");

    /*
    ** CheckChannelSubscriptionbyUser - Checks if a specified channel has a specified user subscribed to it.Intended for use
    **                                  by channel owners. Returns a subscription object which includes the user if that
    **                                  user is subscribed.Requires authentication for the channel.
    **
    **  Note: UserInfo ProfileBanner and ProfileBannerBackgroundColor members are
    **        not used in the UserInfo struct and will be empty.
    */

    CreateReturnTypeClass(SubscriberbyUserValue, SubscriberInfo, "{918C02BC-D78E-43A6-BDB8-7A0A69CDAA32}");

    /*
    ** GetChannelVideos - Gets a list of videos from a specified channel.
    **
    */

    enum class BroadCastType
    {
        Default                         = 0x00,
        Archive                         = 0x01, 
        Highlight                       = 0x02, 
        Upload                          = 0x04,
        ArchiveAndHighlight             = Archive | Highlight,
        ArchiveAndUpload                = Archive | Upload,
        ArchiveAndHighlightAndUpload    = Archive | Highlight | Upload,
        HighlightAndUpload              = Highlight | Upload,
    };

    struct VideoChannelInfo
    {
        AZ_TYPE_INFO(VideoChannelInfo, "{DC2994E0-9462-4D9A-9E0C-7EFBE10E230F}");

        AZStd::string           ID;
        AZStd::string           DisplayName;
        AZStd::string           Name;
    };

    struct FPSInfo
    {
        AZ_TYPE_INFO(FPSInfo, "{EAD04B6C-1A52-4B98-89A0-00FBC9BC0412}");

        FPSInfo() : Chunked(0.0f), High(0.0f), Low(0.0f), Medium(0.0f), Mobile(0.0f) {}
        double                  Chunked;
        double                  High;
        double                  Low;
        double                  Medium;
        double                  Mobile;
    };

    struct PreviewInfo
    {
        AZ_TYPE_INFO(PreviewInfo, "{A193FEED-095A-46B3-9A2D-E231B6B25FCC}");

        AZStd::string           Large;
        AZStd::string           Medium;
        AZStd::string           Small;
        AZStd::string           Template;
    };

    struct ResolutionsInfo
    {
        AZ_TYPE_INFO(ResolutionsInfo, "{A61B7417-B9F2-4BBA-BC5F-FFF8ED0E229D}");

        AZStd::string           Chunked;
        AZStd::string           High;
        AZStd::string           Low;
        AZStd::string           Medium;
        AZStd::string           Mobile;
    };

    struct ThumbnailInfo
    {
        AZ_TYPE_INFO(ThumbnailInfo, "{DED1CACB-3E5E-4AD6-ADAB-E3BB7CE501E4}");

        AZStd::string           Type;
        AZStd::string           Url;
    };

    struct ThumbnailsInfo
    {
        AZ_TYPE_INFO(ThumbnailsInfo, "{81049CE7-FF10-4DD2-8D6C-D6E0E5304289}");

        ThumbnailInfo           Large;
        ThumbnailInfo           Medium;
        ThumbnailInfo           Small;
        ThumbnailInfo           Template;
    };

    struct VideoInfo
    {
        AZ_TYPE_INFO(VideoInfo, "{E70CF32B-3AAD-4646-985F-1B434280E9BD}");

        VideoInfo() : Length(0), Views(0), BroadcastID(0), Type(BroadCastType::Default) {}

        AZ::u64                 Length;
        AZ::u64                 Views;
        AZ::u64                 BroadcastID;
        BroadCastType           Type;
        AZStd::string           CreatedDate;
        AZStd::string           Description;
        AZStd::string           DescriptionHTML;
        AZStd::string           ID;
        AZStd::string           Game;
        AZStd::string           Language;
        AZStd::string           PublishedDate;
        AZStd::string           Status;
        AZStd::string           TagList;
        AZStd::string           Title;
        AZStd::string           URL;
        AZStd::string           Viewable;
        AZStd::string           ViewableAt;
        VideoChannelInfo        Channel;
        FPSInfo                 FPS;
        PreviewInfo             Preview;
        ThumbnailsInfo          Thumbnails;
        ResolutionsInfo         Resolutions;
    };

    using VideoInfoList = AZStd::list<VideoInfo>;

    struct VideoReturn
    {
        AZ_TYPE_INFO(VideoReturn, "{CFB87443-C8FE-437B-B239-7EF34BAF4A56}");

        VideoReturn() : Total(0) {}

        AZ::u64             Total;
        VideoInfoList       Videos;
    };

    CreateReturnTypeClass(VideoReturnValue, VideoReturn, "{DFFC5D09-6B1B-4887-B943-5E386610F597}");

    /*
    ** StartChannelCommercial - Starts a commercial (advertisement) on a specified channel. This is valid only for channels that are Twitch
    **                          partners. You cannot start a commercial more often than once every 8 minutes. The length of the commercial
    **                          (in seconds) is specified in the request body, with a required length parameter. Valid values are 30, 60, 90, 120, 150, and 180.
    **                          There is an error response (422 Unprocessable Entity) if an invalid length is specified, an attempt is made to start a
    **                          commercial less than 8 minutes after the previous commercial, or the specified channel is not a Twitch partner.
    **/

    enum class CommercialLength { T30Seconds, T60Seconds, T90Seconds, T120Seconds, T150Seconds, T180Seconds };

    struct StartChannelCommercialResult
    {
        AZ_TYPE_INFO(StartChannelCommercialResult, "{CF8B0E34-9826-4354-9499-19DE82D1F359}");

        StartChannelCommercialResult() : Duration(0), RetryAfter(0) {}
        AZ::u64         Duration;
        AZ::u64         RetryAfter;
        AZStd::string   Message;
    };

    CreateReturnTypeClass(StartChannelCommercialValue, StartChannelCommercialResult, "{6A5B1B94-48E3-4A33-BF14-CC1B14308F68}");

    /*
    ** GetChannelCommunity -    Gets the community for a specified channel.
    */
    
    struct CommunityInfo
    {
        AZ_TYPE_INFO(CommunityInfo, "{844D73A2-E12F-43F0-B873-79427051AC5E}");

        AZStd::string       ID;
        AZStd::string       AvatarImageURL;
        AZStd::string       CoverImageURL;
        AZStd::string       Description;
        AZStd::string       DescriptionHTML;
        AZStd::string       Language;
        AZStd::string       Name;
        AZStd::string       OwnerID;
        AZStd::string       Rules;
        AZStd::string       RulesHTML;
        AZStd::string       Summary;
    };

    CreateReturnTypeClass(CommunityInfoValue, CommunityInfo, "{E4DB9162-620B-435D-A174-266C5A58AE84}");

    using CommunityInfoList = AZStd::list<CommunityInfo>;

    struct CommunityInfoReturn
    {
        AZ_TYPE_INFO(CommunityInfoReturn, "{33FC4C7E-34F2-4E71-9B48-933728843E6B}");

        CommunityInfoReturn() : Total(0) {}

        AZ::u64             Total;
        CommunityInfoList       Communities;
    }; 

    CreateReturnTypeClass(CommunityInfoReturnValue, CommunityInfoReturn, "{2CA085B8-1141-4E4A-A515-93FE48487E04}");
}
