--[[
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--]]

--Script to test portions of the Twitch Gem

Twitch = {
    Properties = {
        enableLog = {default=true, description="Enables logging from the script"},
        userID = {default="", description="The Twitch User ID for account related requests"},
        oauthID = {default="", description="The OAuth access token for requests"},
        friendRequestID = {default="", description="The ID of a friend request to reject/accept"}
    },

    Internal = {
        twitchUserID = "",
        friendID = "",
        channelID = "",
    },

    twitchUserInfo = UserInfo()
}

function Twitch:OnActivate()
    self.TwitchHandler = TwitchNotifyBus.Connect(self)

    --If the Twitch Application ID/OAuth is not set in the exe, then it must be set here!
    --TwitchRequestBus.Broadcast.SetApplicationID("")

    -- User and Auth Requests --
    local userSetReceipt = ReceiptID()
    TwitchRequestBus.Broadcast.SetUserID(userSetReceipt, self.Properties.userID)

    local tokenSetReceipt = ReceiptID()
    TwitchRequestBus.Broadcast.SetOAuthToken(tokenSetReceipt, self.Properties.oauthID)

    local idReceipt = ReceiptID()
    TwitchRequestBus.Broadcast.RequestUserID(idReceipt)
    
    local tokenReceipt = ReceiptID()
    TwitchRequestBus.Broadcast.RequestOAuthToken(tokenReceipt)

    --local userReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.GetUser(userReceipt)

    -- Friend requests, disabled due to permissions restrictions --
    --local friendNotifReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.GetFriendNotificationCount(friendNotifReceipt, self.Properties.userID)

    --Disabling to prevent unintended account modification
    --local resetNotifReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.ResetFriendsNotificationCount(resetNotifReceipt, self.Properties.userID)

    --local getFriendsReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.GetFriends(getFriendsReceipt, self.Properties.userID, "")

    --Note: Creating/Accepting/Declining friend requests requires the friend ID as a parameter--
    --Disabling to prevent unintended account modification
    --local acceptFriendReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.AcceptFriendRequest(acceptFriendReceipt, self.Properties.friendRequestID)

    --Disabling to prevent unintended account modification
    --local createFriendReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.CreateFriendRequest(createFriendReceipt, self.Properties.friendRequestID)

    --Disabling to prevent unintended account modification
    --local declineFriendReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.DeclineFriendRequest(declineFriendReceipt, self.Properties.friendRequestID)

    -- Rich presence requests, disabled due to permissions restrictions -- 
    --Disabling to prevent unintended account modification
    --local updatePresenceStatusReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.UpdatePresenceStatus(updatePresenceStatusReceipt, PresenceAvailability.Unknown, PresenceActivityType.Unknown, "")

    --local getPresenceStatusReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.GetPresenceStatusofFriends(getPresenceStatusReceipt)

    --local getPresenceSettingsReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.GetPresenceSettings(getPresenceSettingsReceipt)

    --Disabling to prevent unintended account modification
    --local updatePresenceSettingsReceipt = ReceiptID()
    --TwitchRequestBus.Broadcast.UpdatePresenceSettings(updatePresenceSettingsReceipt, false, false)

    -- Channel requests --
    local getChannelReceipt = ReceiptID()
    TwitchRequestBus.Broadcast.GetChannel(getChannelReceipt)
end

function Twitch:TestFriends()
    if self.Internal.friendID ~= "" then
        local getFriendStatusReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetFriendStatus(getFriendStatusReceipt, self.Properties.userID, self.Internal.friendID)

        local getFriendReqsReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetFriendRequests(getFriendReqsReceipt, "")

        local getFriendRecsReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetFriendRecommendations(getFriendRecsReceipt, self.Properties.userID)
    end
end

function Twitch:TestChannels()
    if self.Internal.channelID ~= "" then

        local getChannelIDReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetChannelbyID(getChannelIDReceipt, self.Internal.channelID)

        --Disabling to prevent unintended account modification
        --local updateChannelReceipt = ReceiptID()
        --local updateInfo = ChannelUpdateInfo()
        --updateInfo.Status.Value = "This channel is cool x" .. os.clock()
        --Twitch:TryAndLog(updateInfo.Status.Value)
        --TwitchRequestBus.Broadcast.UpdateChannel(updateChannelReceipt, updateInfo)

        local getChannelEditorsReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetChannelEditors(getChannelEditorsReceipt, self.Internal.channelID)

        local getChannelFollowersReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetChannelFollowers(getChannelFollowersReceipt, self.Internal.channelID, "", 0)

        local getChannelTeamsReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetChannelTeams(getChannelTeamsReceipt, self.Internal.channelID)

        --Testing subs requires a channel that supports subscriptions
        --local getChannelSubsReceipt = ReceiptID()
        --TwitchRequestBus.Broadcast.GetChannelSubscribers(getChannelSubsReceipt, self.Internal.channelID, 0)

        --Check if friend is subbed to channel
        --local getChannelSubByUserReceipt = ReceiptID()
        --TwitchRequestBus.Broadcast.CheckChannelSubscriptionbyUser(getChannelSubByUserReceipt, self.Internal.channelID, self.Properties.userID)

        local getChannelVidsReceipt = ReceiptID()
        TwitchRequestBus.Broadcast.GetChannelVideos(getChannelVidsReceipt, self.Internal.channelID, BroadCastType.Default, "en", 0)

        --Disabling to prevent unintended account modification
        --local startChannelCommercialReceipt = ReceiptID()
        --TwitchRequestBus.Broadcast.StartChannelCommercial(startChannelCommercialReceipt, self.Internal.channelID, CommercialLength.T30Seconds)

        --Disabling to prevent unintended account modification
        --local resetChannelStreamKeyReceipt = ReceiptID()
        --TwitchRequestBus.Broadcast.ResetChannelStreamKey(resetChannelStreamKeyReceipt, self.Internal.channelID)

    end
end

function Twitch:OnDeactivate()
    if self.TwitchHandler ~= nil then
        self.TwitchHandler:Disconnect()
    end
end

function Twitch:TryAndLog(logValue)
    if self.Properties.enableLog then  
        Debug.Log(logValue)
    end
end 

-- User and Auth notifications 
function Twitch:UserIDNotify(result)
    self.Properties.userID = result.Value
    Twitch:TryAndLog("UserIDNotify: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:OAuthTokenNotify(result) 
    self.Properties.oauthID = result.Value
    Twitch:TryAndLog("OAuthTokenNotify: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetUser(result)
    self.Internal.twitchUserID = result.Value.ID
    self.twitchUserInfo = result.Value
    Twitch:TryAndLog("GetUser: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

-- Friend notifications

function Twitch:ResetFriendsNotificationCountNotify(result)
    Twitch:TryAndLog("ResetFriendsNotificationCountNotify: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetFriendNotificationCount(result)
    Twitch:TryAndLog("GetFriendNotificationCount: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetFriendRecommendations(result)
    Twitch:TryAndLog("GetFriendRecommendations: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetFriends(result)
    Twitch:TryAndLog("GetFriends: " ..tostring(result))

    assert(result.Result == ResultCode.Success)
    if #result.Value.Friends > 0 then
        self.Internal.friendID = result.Value.Friends[0].User.ID
    end
    self:TestFriends()
end

function Twitch:GetFriendStatus(result)
    Twitch:TryAndLog("GetFriendStatus: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:AcceptFriendRequest(result)
    Twitch:TryAndLog("AcceptFriendRequest: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetFriendRequests(result)
    Twitch:TryAndLog("GetFriendRequests: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:CreateFriendRequest(result)
    Twitch:TryAndLog("CreateFriendRequest: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:DeclineFriendRequest(result)
    Twitch:TryAndLog("DeclineFriendRequest: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

-- Rich Presence
function Twitch:UpdatePresenceStatus(result)
    Twitch:TryAndLog("UpdatePresenceStatus: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetPresenceStatusofFriends(result)
    Twitch:TryAndLog("GetPresenceStatusofFriends: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetPresenceSettings(result)
    Twitch:TryAndLog("GetPresenceSettings: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:UpdatePresenceSettings(result)
    Twitch:TryAndLog("UpdatePresenceSettings: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

-- Channels
function Twitch:GetChannel(result)
    Twitch:TryAndLog("GetChannel: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
    self.Internal.channelID = result.Value.Id
    self:TestChannels()
end

function Twitch:GetChannelbyID(result)
    Twitch:TryAndLog("GetChannelbyID: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:UpdateChannel(result)
    Twitch:TryAndLog("UpdateChannel: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetChannelEditors(result)
    Twitch:TryAndLog("GetChannelEditors: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetChannelFollowers(result)
    Twitch:TryAndLog("GetChannelFollowers: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetChannelTeams(result)
    Twitch:TryAndLog("GetChannelTeams: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetChannelSubscribers(result)
    Twitch:TryAndLog("GetChannelSubscribers: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:CheckChannelSubscriptionbyUser(result)
    Twitch:TryAndLog("CheckChannelSubscriptionbyUser: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:GetChannelVideos(result)
    Twitch:TryAndLog("GetChannelVideos: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:StartChannelCommercial(result)
    Twitch:TryAndLog("StartChannelCommercial: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

function Twitch:ResetChannelStreamKey(result)
    Twitch:TryAndLog("ResetChannelStreamKey: " ..tostring(result))
    assert(result.Result == ResultCode.Success)
end

return Twitch
