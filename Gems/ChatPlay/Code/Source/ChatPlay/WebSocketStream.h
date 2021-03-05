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
#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_WEBSOCKETSTREAM_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_WEBSOCKETSTREAM_H
#pragma once

#include <AzCore/std/smart_ptr/weak_ptr.h>

#include "IStreamHandler.h"

// This class keeps track of and handles a WebSocket session and its stream of messages.
// Given the raw messages from the server and a callback to communicate with it, this 
// handler will attempt to execute the initial WebSocket handshake before handling the
// encoding (framing) and decoding of messages to/from the server.
//
// Can act as a translation layer for another IStreamHandler.
//
class WebSocketStream : public IStreamHandler
{
public:
    // If provided with an IStreamHandler, will pass events through accordingly.
    WebSocketStream(const char* address, const AZStd::shared_ptr<IStreamHandler>& streamHandler);

    // Event handling functions (see IStreamHandler.h)
    virtual HandlerState OnConnect() override;
    virtual HandlerState OnMessage(const char* message, size_t messageLength) override;

    // Returns true if message was successfully framed and passed on to sendCallback.
    // Note: this will only succeed if the WebSocket handshake was successful.
    virtual bool SendMessage(const char* input, size_t inputLength) override;
    bool SendMessageFromStream(const char* input, size_t inputLength);

    void SetSendFunction(const SendMessageCallback& send) override;

    AZStd::weak_ptr<IStreamHandler> GetStreamHandler() { return m_streamHandler; };

private:
    enum FrameType {
        ERROR_FRAME = 0xFF00,
        INCOMPLETE_FRAME = 0xFE00,

        OPENING_FRAME = 0x3300,
        CLOSING_FRAME = 0x3400,

        INCOMPLETE_TEXT_FRAME = 0x01,
        INCOMPLETE_BINARY_FRAME = 0x02,

        TEXT_FRAME = 0x81,
        BINARY_FRAME = 0x82,

        PING_FRAME = 0x19,
        PONG_FRAME = 0x1A
    };

    // Construct the header for the websocket handshake
    AZStd::string PrepareWebSocketHeader();

    // Helper functions for encoding and decoding messages
    static size_t frameSize(size_t msgLength, bool mask);
    static size_t makeFrame(FrameType frameType, const char* msg, size_t msgLength, bool mask, char* buffer, size_t bufferLength);
    static FrameType getFrame(const char* inBuffer, size_t inLength, char* buffer, size_t bufferLength, size_t* outLength);

    // WebSocket response
    const char* RPL_SERVER_HANDSHAKE = " 101 Switching Protocols";

    // Members
    const char* m_address;

    AZStd::shared_ptr<IStreamHandler> m_streamHandler;

    // State of handler
    bool m_handshook;

    SendMessageCallback m_send;
};

inline void WebSocketStream::SetSendFunction(const SendMessageCallback& send)
{
    m_send = send;
}

#endif    /* CRYINCLUDE_CRYACTION_CHATPLAY_WEBSOCKETSTREAM_H */