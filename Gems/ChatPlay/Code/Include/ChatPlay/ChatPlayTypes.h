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
// Twitch ChatPlay Types
#pragma once

#include <regex>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace ChatPlay
{
    /******************************************************************************/
    // ChatPlay Channel Types
    //

    // Single enum to represent the connection state
    enum class ConnectionState
    {
        // This state is used after the connection to a chat channel is fully shutdown
        Disconnected,

        // This state covers all of:
        // * Pre-connection setup
        // * Opening a TCP stream
        // * Protocol handshake with the server
        // * Authenticating
        // * Joining the channel
        Connecting,

        // This state covers:
        // * Fully connected to channel
        // * In the process of leaving a channel
        Connected,

        // This state covers fatal errors; this state implies Disconnected and will
        // persist until a new connection is started
        Error,

        // This state indicates all connections failed
        Failed
    };

    // Callback for state changes
    typedef AZStd::function<void(ConnectionState)> StateCallback;

    // Callback for keyword hits
    // The parameters passed to the callback are:
    //  match    : the matched string
    //  username : username of the sender
    typedef AZStd::function<void(const AZStd::string&, const AZStd::string&)> KeywordCallback;

    // The callback token is used to assist with de-registration of callbacks
    typedef AZ::u64 CallbackToken;

    // Token for identifying Whisper notifications
    typedef AZ::u64 WhisperToken;

    // Enum for whisper result
    enum class WhisperResult
    {
        Success,
        MissingOAuthToken,
        ConnectionError,
        AuthenticationError
    };

    // Typedef for whisper callbacks
    typedef AZStd::function<void(WhisperResult)> WhisperCallback;

    /******************************************************************************/
    // ChatPlay Vote Types
    //

    // Structure for defining a vote option
    struct VoteOption
    {
    public:
        AZ_TYPE_INFO(VoteOption, "{6A0344D2-32BD-4047-A7C8-0ED8921D7CBE}");

        VoteOption(AZStd::string name)
            : m_name(name)
            , m_count(0)
            , m_enabled(false)
        {
            if (m_name.empty())
            {
                AZ_Warning("ChatPlay", false, "Created a vote option with an empty name.");
            }
        }

        const AZStd::string& GetName() const { return m_name; }

        int  GetCount() const    { return m_count; }
        void SetCount(int count) { m_count = count; }

        bool GetEnabled() const       { return m_enabled; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        VoteOption& operator=(const VoteOption& option)
        {
            m_name    = option.GetName();
            m_count   = option.GetCount();
            m_enabled = option.GetEnabled();

            return *this;
        }

    private:
        AZStd::string m_name;
        int           m_count;
        bool          m_enabled;
    };

    /******************************************************************************/
    // ChatPlay Utils
    //

    // Utils class for shared helper methods
    class ChatPlayUtils
    {
    public:

        // Returns true if the given Channel name is valid, false otherwise
        static bool IsValidChannelName(const AZStd::string& channelId)
        {
            // For now, we just check if the channel name is not empty
            return !channelId.empty();
        }
    };
}

// Setting up our Enums to be able to be reflected into the BehaviorContext.
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(ChatPlay::ConnectionState, "{B19D928B-938F-4862-BF5D-5E7126A37396}");
    AZ_TYPE_INFO_SPECIALIZE(ChatPlay::WhisperResult, "{7D17D951-BBE7-4928-92F5-424E204EF576}");
}