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

#include <AzCore/EBus/EBus.h>
#include <ChatPlay/ChatPlayTypes.h>

namespace ChatPlay
{
    class IBroadcast;
    class IHttpRequestManager;

    class ChatPlayRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // To be deprecated
        virtual IBroadcast* GetBroadcastAPI() { return nullptr; }
        virtual IHttpRequestManager* GetHttpRequestManager() { return nullptr; }

        /******************************************************************************/
        // Events
        //

        // This will create a ChatChannel object if one does not exist for this ChannelId
        // Returns true if channel was successfully created (or if it already existed), false otherwise
        // There are no restrictions on channelId -- you'll find out if it is invalid when you try connecting to it later (through RegisterConnectionStateChange)
        virtual bool CreateChannel(const AZStd::string& channelId) = 0;

        // Disconnects and destroys a ChatChannel
        virtual void DestroyChannel(const AZStd::string& channelId) = 0;

        // Function that iterates through the internal list of ChatChannels and calls disconnect on each one
        virtual void DisconnectAll() = 0;

        // Registers the credential pair (username, oauth token)
        // Overwrites the previously stored oauth token if the username was already registered
        virtual void RegisterCredentials(const AZStd::string& username, const AZStd::string& oauthToken) = 0;

        // Unregisters the credential pair for the given username
        virtual void UnregisterCredentials(const AZStd::string& username) = 0;

        // Unregister all stored credential
        virtual void UnregisterAllCredentials() = 0;

        // Sends a whisper (private message) to recipient on behalf of sender using registered credentials
        // Note: The whisper result only indicates if the message was sent; it doesn't indicate receipt
        virtual void SendWhisperWithCallback(
            const AZStd::string& sender,
            const AZStd::string& recipient,
            const AZStd::string& message,
            const WhisperCallback& callback) = 0;

        // Sends a whisper (private message) to recipient on behalf of sender using registered credentials
        // The returned token allows for listening on the ChatPlayNotificationBus for when the message was sent
        virtual WhisperToken SendWhisper(
            const AZStd::string& sender,
            const AZStd::string& recipient,
            const AZStd::string& message) = 0;

        // This will create a Vote object if one does not exist for this VoteId
        // Returns true if vote was successfully created (or if it already existed), false otherwise
        virtual bool CreateVote(const AZStd::string& voteId) = 0;

        // Disconnects and destroys a ChatChannel
        virtual void DestroyVote(const AZStd::string& voteId) = 0;
    };

    class ChatPlayNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnWhisperSent(WhisperResult result) = 0;
    };

    class ChatPlayChannelRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        // Address by ChannelID
        using BusIdType = AZStd::string;

        /******************************************************************************/
        // Events
        //
        
        // Starts a connection; idempotent if the ChatChannel is already connected or is connecting
        virtual void Connect() = 0;

        // Starts disconnecting; idempotent is the ChatChannel is already disconnected or is disconnecting
        virtual void Disconnect() = 0;

        // Returns the cached state of the connection (actual connection state may differ; use RegisterConnectionStateChange to receive async notifications)
        virtual ConnectionState GetConnectionState() = 0;

        // Register a callback to be invoked when the connection state changes
        // Returns a token that can be used to unregister the callback later
        // Note: callbacks will be executed when IChatPlay::DispatchEvents is called
        virtual CallbackToken RegisterConnectionStateChange(const StateCallback& callback) = 0;

        // Unregisters the specified callback
        virtual void UnregisterConnectionStateChange(CallbackToken token) = 0;

        // Register a callback to be invoked when the specified keyword is used in the channel
        // Returns a token that can be used to unregister the callback later
        // Note: callbacks will be executed when IChatPlay::DispatchEvents is called
        virtual CallbackToken RegisterKeyword(const AZStd::string& keyword, const KeywordCallback& callback) = 0;

        // Unregisters the specified callback
        virtual void UnregisterKeyword(CallbackToken token) = 0;

    };

    class ChatPlayChannelNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        // Address by ChannelID
        using BusIdType = AZStd::string;

        /******************************************************************************/
        // Events
        //

        // Implement this to listen for a channel's changes in connection state
        virtual void OnConnectionStateChanged(ConnectionState /*state*/) {}

        // Implement this to listen for keyword matches
        //  keyword  : the original keyword (or regex) that was matched
        //  match    : the actual matched string
        //  username : the username of the user who triggered the match
        virtual void OnKeywordMatched(const AZStd::string& /*keyword*/, const AZStd::string& /*match*/, const AZStd::string& /*username*/) {}

    };

    class ChatPlayVoteRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        // Address by ChannelID
        using BusIdType = AZStd::string;

        /******************************************************************************/
        // Events
        //

        // Add a new option
        // Returns true if a new option was successfully added, false otherwise
        //
        // Note: The name should be composed of only characters that can appear in
        // a normal Chat message.
        virtual bool AddOption(const AZStd::string& name) = 0;

        // Removes an option from the vote entirely.
        //
        // This is different to IChatVoteOption::SetEnable in that the option will
        // cease to exist and future calls to GetOption will return nullptr.
        virtual void RemoveOption(const AZStd::string& name) = 0;

        // Configure an existing option
        virtual void ConfigureOption(const AZStd::string& optionName, int count, bool enabled) = 0;

        // Checks if option exists
        // Returns true if the option exists (was added prior), false otherwise
        virtual bool OptionExists(const AZStd::string& name) = 0;

        // Get the vote count of an existing option
        // Returns 0 if the option doesn't exist
        virtual int GetOptionCount(const AZStd::string& optionName) = 0;

        // Set the vote count of an existing option
        virtual void SetOptionCount(const AZStd::string& optionName, int count) = 0;

        // Get the enabled state of an existing option
        // Returns false if the option doesn't exist
        virtual bool GetOptionEnabled(const AZStd::string& optionName) = 0;

        // Set the enabled state of an existing option
        virtual void SetOptionEnabled(const AZStd::string& optionName, bool enabled) = 0;

        // Changes the ChatPlay channel that the vote is connected to.
        //
        // Note: Holds a shared reference to the channel until ClearChannel() is
        // invoked or a further call to SetChannel is made.
        //
        // Returns true if the channel was found and false otherwise.
        virtual bool SetChannel(const AZStd::string& name) = 0;

        // Clear any channel reference stored by the vote
        virtual void ClearChannel() = 0;

        // Visitor pattern: Visit all options and invoke the visitor function
        //
        // NOTE: It is unsafe to modify the current option set from inside the
        // visitor; i.e. do not call AddOption() or RemoveOption().
        virtual void Visit(const AZStd::function<void(VoteOption& option)>& visitor) = 0;

        virtual void SetEnableStateAll(bool state) = 0;
        virtual void SetCountAll(int count) = 0;
        virtual void SetVoterLimiting(bool limit) = 0;

        virtual void ResetVotedList() = 0;
    };

    // Request Buses
    using ChatPlayRequestBus = AZ::EBus<ChatPlayRequests>;
    using ChatPlayChannelRequestBus = AZ::EBus<ChatPlayChannelRequests>;
    using ChatPlayVoteRequestBus = AZ::EBus<ChatPlayVoteRequests>;

    // Notification Buses
    using ChatPlayNotificationBus = AZ::EBus<ChatPlayNotifications>;
    using ChatPlayChannelNotificationBus = AZ::EBus<ChatPlayChannelNotifications>;

} // namespace ChatPlay
