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

#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_ICHATPLAYSTREAM_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_ICHATPLAYSTREAM_H
#pragma once

#include <AzCore/std/functional.h>

// Interface for ChatPlay message stream handlers
class IStreamHandler
{
public:
    enum class HandlerState
    {
        AWAITING_RESPONSE,
        UNHANDLED_RESPONSE,
        CONNECTED,
        MESSAGE_RECEIVED,
        MESSAGE_SENT,
        HANDLER_ERROR,
    };

    typedef AZStd::function<void(const char* message, size_t messageLength)> SendMessageCallback;

    virtual ~IStreamHandler() = default;

    // Initial connection handler
    virtual HandlerState OnConnect() = 0;

    // Handler for message receipt event
    virtual HandlerState OnMessage(const char* message, size_t messageLength) = 0;

    // Prepares input accordingly
    virtual bool SendMessage(const char* input, size_t inputLength) = 0;

    virtual void SetSendFunction(const SendMessageCallback& send) = 0;
};

#endif // CRYINCLUDE_CRYACTION_CHATPLAY_ICHATPLAYSTREAM_H
