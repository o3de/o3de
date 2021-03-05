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
#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_IRCSTREAM_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_IRCSTREAM_H
#pragma once

#include "IStreamHandler.h"

// This class keeps track of and handles the stream of messages to/from an IRC server.
// Given the raw messages from the IRC server and a callback to communicate with it,
// this handler will attempt to authenticate using the provided credentials and and join 
// the given channel.
//
class IRCStream : public IStreamHandler
{
public:
    typedef AZStd::function<void(const AZStd::string& message)> IRCMessageCallback;

    // Channel can be set to null if joining a channel is not required.
    // (e.g. for sending whispers)
    IRCStream(const char* nick, const char* pass, const char* channel);

    // Event handling functions (see IStreamHandler.h)
    virtual HandlerState OnConnect() override;
    virtual HandlerState OnMessage(const char* message, size_t messageLength) override;

    // Returns true if message was passed on to sendCallback, false otherwise.
    // Notes:
    // * Sending a message will only succeed if a valid nick/pass pair was provided
    //   and connection was successful
    // * OnMessage() will return a MESSAGE_SENT state if the server is still
    //   responsive after the message was sent; this does not guarantee the message
    //   was delivered
    virtual bool SendMessage(const char* message, size_t messageLength) override;

    void SetSendFunction(const SendMessageCallback& send) override;
    void SetMessageFunction(const IRCMessageCallback& message); // IRC messages

private:

    // RFC2812
    const char* RPL_WELCOME = " 001 ";
    const char* RPL_ENDOFNAMES = " 366 ";
    const char* RPL_PING = "PING";
    const char* RPL_PONG = "PONG";
    const char* RPL_JOIN_RESPONSE = "JOIN #";

    // Members
    const char* m_nick;
    const char* m_pass;
    const char* m_channel;

    // State of handler
    bool m_authenticated;
    bool m_joined;
    bool m_messageSent;

    SendMessageCallback m_send;
    IRCMessageCallback m_message;
};

inline void IRCStream::SetSendFunction(const SendMessageCallback& send)
{
    m_send = send;
}

inline void IRCStream::SetMessageFunction(const IRCMessageCallback& message)
{
    m_message = message;
}

#endif /* CRYINCLUDE_CRYACTION_CHATPLAY_IRCSTREAM_H */