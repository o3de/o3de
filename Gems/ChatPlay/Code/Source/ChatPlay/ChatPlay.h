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
// Twitch ChatPlay interfaces
#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAY_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAY_H
#pragma once

#include <ChatPlay/ChatPlayTypes.h>
#include <ChatPlay/ChatPlayBus.h>

namespace ChatPlay
{

    // Interface for interacting with Chat Channels obtained from Twitch ChatPlay
    class ChatChannel
        : public ChatPlayChannelRequestBus::Handler
    {
    public:

        virtual ~ChatChannel() = default;

        // Channels always have an Id and the Id does not change
        virtual const AZStd::string& GetChannelId() const = 0;
    };

    class ChatPlayVoteManager;

    // Twitch ChatPlay interface
    class ChatPlay
    {
    public:
        virtual ~ChatPlay() = default;

        // Factory function for creating ChatPlay
        static AZStd::shared_ptr<ChatPlay> CreateInstance();

        // This will create a ChatChannel object if one does not exist for this channelId and adds it to our managed list
        // There are no restrictions on channelId -- you'll find out if it is invalid when you try connecting to it later (through RegisterConnectionStateChange)
        virtual AZStd::weak_ptr<ChatChannel> GetChatChannel(const AZStd::string& channelId) = 0;

        // Disconnects and destroys a channel, removing it from our managed list
        virtual void DestroyChatChannel(const AZStd::string& channelId) = 0;

        // Function that iterates through the internal list of ChatChannels and calls disconnect on each one
        virtual void DisconnectAll() = 0;

        // On the thread on which this is invoked, all waiting ChatChannel callbacks will be executed
        virtual AZStd::size_t DispatchEvents() = 0;

        // Registers the credential pair (username, oauth token)
        // Overwrites the previously stored oauth token if the username was already registered
        virtual void RegisterCredentials(const AZStd::string& username, const AZStd::string& oauthToken) = 0;

        // Unregisters the credential pair for the given username
        virtual void UnregisterCredentials(const AZStd::string& username) = 0;

        // Unregister all stored credential
        virtual void UnregisterAllCredentials() = 0;

        // Sends a whisper (private message) to recipient on behalf of sender using registered credentials
        // Note: The whisper result only indicates if the message was sent; it doesn't indicate receipt
        virtual void SendWhisper(
            const AZStd::string& sender,
            const AZStd::string& recipient,
            const AZStd::string& message,
            const WhisperCallback& callback) = 0;

        // Returns an instance of the ChatPlay Vote Manager that can be used to
        // interact with the voting features.
        virtual ChatPlayVoteManager* GetVoteManager() const = 0;
    };

    // Twitch Chat Play Votes are containers for Options and allow high-level
    // control over a specific Vote.
    class ChatPlayVote
        : public ChatPlayVoteRequestBus::Handler
    {
    public:
        virtual ~ChatPlayVote() = default;

        // The Vote name is used as an identifier within the system.
        // There are no constraints on the format or content of the name.
        virtual const AZStd::string& GetName() const = 0;
    };

    // Twitch Chat Play Vote Manager
    class ChatPlayVoteManager
    {
    public:
        virtual ~ChatPlayVoteManager() = default;

        // Gets a vote by id or creates and adds one to a managed list if it didn't exist.
        // Returns a weak reference if the vote was found or created
        virtual AZStd::weak_ptr<ChatPlayVote> GetVote(const AZStd::string& voteId) = 0;

        // Destroys a vote, removing it from our managed list
        virtual void DestroyVote(const AZStd::string& voteId) = 0;
    };

}

#endif // CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAY_H
