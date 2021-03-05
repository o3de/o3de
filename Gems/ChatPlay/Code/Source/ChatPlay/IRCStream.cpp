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
#include "ChatPlay_precompiled.h"
#include "IRCStream.h"

#include <sstream>

IRCStream::IRCStream(const char* nick, const char* pass, const char* channel)
    : m_nick(nick)
    , m_pass(pass)
    , m_channel(channel)
{
    m_authenticated = false;
    m_joined = false;
}

IStreamHandler::HandlerState IRCStream::OnConnect()
{
    if (m_send)
    {
        std::stringstream stream;
        stream << "PASS " << m_pass << "\r\n";
        m_send(stream.str().c_str(), stream.str().length());

        stream.str(std::string());
        stream.clear();

        stream << "NICK " << m_nick << "\r\n";
        m_send(stream.str().c_str(), stream.str().length());

        return IStreamHandler::HandlerState::AWAITING_RESPONSE;
    }

    return IStreamHandler::HandlerState::HANDLER_ERROR;
}

IStreamHandler::HandlerState IRCStream::OnMessage(const char* message, size_t messageLength)
{
    AZStd::string_view messageView{ message, messageLength };
    if (messageLength > 0)
    {
        if (!m_authenticated)
        {
            if (messageView.find(RPL_WELCOME) != AZStd::string_view::npos)
            {
                m_authenticated = true;

                std::stringstream stream;

                if (m_channel)
                {
                    stream << "JOIN #" << m_channel << "\r\n";
                    m_send(stream.str().c_str(), stream.str().length());

                    return IStreamHandler::HandlerState::AWAITING_RESPONSE;
                }
                else
                {
                    m_joined = true;
                    return IStreamHandler::HandlerState::CONNECTED;
                }
            }
            else if (messageView.find("Error logging in") != AZStd::string_view::npos )
            {
                return HandlerState::HANDLER_ERROR;
            }
        }
        else if (!m_joined)
        {
            if (messageView.find(RPL_JOIN_RESPONSE) != AZStd::string_view::npos)
            {
                m_joined = true;
                return IStreamHandler::HandlerState::CONNECTED;
            }
        }
        else if (messageView.find(RPL_PING) != AZStd::string_view::npos)
        {
            AZStd::string pong = AZStd::string::format("%s\r\n", RPL_PONG);
            m_send(pong.c_str(), pong.size());
            
            if (m_messageSent)
            {
                m_messageSent = false;
                return IStreamHandler::HandlerState::MESSAGE_SENT;
            }

            return IStreamHandler::HandlerState::AWAITING_RESPONSE;
        }
        else if (messageView.find(RPL_PONG) != AZStd::string_view::npos)
        {
            if (m_messageSent)
            {
                m_messageSent = false;
                return IStreamHandler::HandlerState::MESSAGE_SENT;
            }
        }
        else
        {
            if (m_message)
            {
                m_message(AZStd::string(message, messageLength));
            }

            return IStreamHandler::HandlerState::MESSAGE_RECEIVED;
        }
    }

    return IStreamHandler::HandlerState::UNHANDLED_RESPONSE;
}

bool IRCStream::SendMessage(const char* message, size_t messageLength)
{
    if (m_authenticated && m_joined && m_send)
    {
        m_send(message, messageLength);
        m_messageSent = true;

        // Send a ping to force the server to respond
        AZStd::string ping = AZStd::string::format("%s\r\n", RPL_PING);
        m_send(ping.c_str(), ping.size());

        return true;
    }

    return false;
}
