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
// Implementation of Twitch ChatPlay feature
#include "ChatPlay_precompiled.h"

#include <AzCore/std/containers/set.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>
#include <HttpRequestor/HttpRequestorBus.h>

#include "ChatPlay.h"
#include "ChatPlaySystemComponent.h"
#include "ChatPlayCVars.h"
#include "IRCStream.h"
#include "LibDyad.h"
#include "WebSocketStream.h"


namespace ChatPlay
{

    class ChatPlayImpl;

    /******************************************************************************/
    // Implementation of ChatChannel
    //
    class ChatChannelImpl
        : public ChatChannel
        , public AZStd::enable_shared_from_this < ChatChannelImpl >
    {
    public:
        ChatChannelImpl(const AZStd::string& channelId, ChatPlayImpl* chatPlay);

        ~ChatChannelImpl() override;

        const AZStd::string& GetChannelId() const;

        void Connect() override;
        void Disconnect() override;

        ConnectionState GetConnectionState() override;

        CallbackToken RegisterConnectionStateChange(const StateCallback& callback) override;
        void UnregisterConnectionStateChange(CallbackToken token) override;

        CallbackToken RegisterKeyword(const AZStd::string& keyword, const KeywordCallback& callback) override;
        void UnregisterKeyword(CallbackToken token) override;

    private:

        static CallbackToken ms_callbackToken;

        // Channel Id can only be set on construction; makes life more predictable
        const AZStd::string m_channelId;

        // Reference back to ChatPlay instance
        ChatPlayImpl* m_chatPlay;

        // Maps tokens to state change callbacks (Used on Dispatch Event Thread)
        AZStd::map<CallbackToken, StateCallback> m_stateCallbacks;

        // Maps tokens to keyword callbacks (Used on Dispatch Event Thread)
        AZStd::map<CallbackToken, KeywordCallback> m_keywordCallbacks;

        // Maps tokens for keyword callbacks to the keyword they represent (Used on Dispatch Event Thread)
        AZStd::map<CallbackToken, AZStd::string> m_tokenToKeyword;

        // Maps keywords to the callback token; reverse of m_keywordCallbacks (Used on Dispatch Event Thread)
        AZStd::multimap<AZStd::string, CallbackToken> m_keywordTokens;

        // Requires synchronized access (with m_keywordLock)
        AZStd::unordered_map<AZStd::string, AZStd::regex> m_keywords;

        // Protects the keyword list (see m_keywords)
        AZStd::mutex m_keywordLock;

        // May only be changed by the DISPATCH EVENT THREAD
        ConnectionState m_connectionState;

        // The epoch is used to tell which instance of a chatbot we're receiving messages from
        dyad::StreamId m_epoch;

        // List of IRC server hosts and ports associated with this channel
        HostInfoList m_hostInfoList;

        // Index of host we are connected to
        int m_connectedHostIndex;

        // Bool to keep track of if we managed a successful connection
        bool m_successfulConnection;

        // Message stream handler (IRC or WebSocket)
        AZStd::unique_ptr<IStreamHandler> m_streamHandler;

        // Sends a notification to the Dispatch Event thread about state changes
        void PostConnectionState(dyad::StreamId epoch, ConnectionState state);

        // Changes the connection state (for use on Dispatch Event Thread)
        void ChangeConnectionState(dyad::StreamId, ConnectionState state);

        void OnChatbotReceived(dyad::StreamId epoch, const AZStd::string& msg);

        // Event to be raised when a keyword is detected
        void KeywordEvent(dyad::StreamId epoch, const AZStd::string& keyword, const AZStd::string& match, const AZStd::string& username);

        void OnStreamCreate(dyad::CDyadStream&);
        void OnStreamEvent(dyad::CDyadEvent&);

        // Processes the data returned by the Http request and stores the list of server hosts and ports in m_hostAndPortList
        void ProcessHostList(const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode);

        // Helper function for processing returned host list
        bool PopulateHostInfoList(HostInfoList& hostInfoList, const Aws::Utils::Json::JsonView& jsonValue, bool isWebsocket);

        // Construct the API URL for the server list request
        AZStd::string MakeServerListURL();
    };

    /******************************************************************************/
    // Implementation of ChatPlay
    //

    class ChatPlayImpl
        : public ChatPlay
        , public AZStd::enable_shared_from_this < ChatPlayImpl >
    {
    public:
        ChatPlayImpl();
        ~ChatPlayImpl() override;

        const AZStd::shared_ptr<ChatPlayCVars>& GetVars();
        const std::shared_ptr<dyad::IDyad>& GetDyad();

        AZStd::weak_ptr<ChatChannel> GetChatChannel(const AZStd::string& channelId) override;
        void DestroyChatChannel(const AZStd::string& channelId) override;

        void DisconnectAll() override;

        AZStd::size_t DispatchEvents() override;

        typedef AZStd::function<void()> ChatPlayEvent;
        void RegisterEvent(const ChatPlayEvent& event);

        void RegisterCredentials(const AZStd::string& username, const AZStd::string& oauthToken) override;
        void UnregisterCredentials(const AZStd::string& username) override;
        void UnregisterAllCredentials() override;

        // Returns the registered oauth token associated with the given username
        // Returns null if no credentials registered for given username
        const char* GetOAuthToken(const AZStd::string& username);

        void SendWhisper(
            const AZStd::string& sender, 
            const AZStd::string& recipient, 
            const AZStd::string& message,
            const WhisperCallback& callback) override;

        ChatPlayVoteManager* GetVoteManager() const override;

    private:

        // Reference to CVar manager
        AZStd::shared_ptr<ChatPlayCVars> m_vars;

        // Dictionary of pointers to ChatChannels
        AZStd::map<AZStd::string, AZStd::shared_ptr<ChatChannel>> m_channelMap;

        // Protects access to the channels map
        AZStd::mutex m_channelLock;

        // Protects access to the event queue
        AZStd::mutex m_eventLock;

        // Placeholder: Events as callbacks
        AZStd::vector<ChatPlayEvent> m_events;

        // Reference to dyad instance
        std::shared_ptr<dyad::IDyad> m_dyad;

        // Container for Twitch IRC credentials
        AZStd::map<AZStd::string, AZStd::string> m_credentialMap;

        // Vote manager
        AZStd::unique_ptr<ChatPlayVoteManager> m_voteManager;
    };

    /******************************************************************************/
    // Implementation of ChatPlayVote
    //

    class ChatPlayVoteImpl
        : public ChatPlayVote
    {
    public:
        ChatPlayVoteImpl(const AZStd::string& name, ChatPlayImpl* chatplay);
        ~ChatPlayVoteImpl();

        const AZStd::string& GetName() const override;

        bool AddOption(const AZStd::string& name) override;
        void RemoveOption(const AZStd::string& name) override;
        void ConfigureOption(const AZStd::string& optionName, int count, bool enabled) override;
        bool OptionExists(const AZStd::string& name) override;

        int  GetOptionCount(const AZStd::string& optionName) override;
        void SetOptionCount(const AZStd::string& optionName, int count) override;
        bool GetOptionEnabled(const AZStd::string& optionName) override;
        void SetOptionEnabled(const AZStd::string& optionName, bool enabled) override;

        bool SetChannel(const AZStd::string& name) override;
        void ClearChannel() override;

        void Visit(const AZStd::function<void(VoteOption& option)>& visitor) override;

        void SetEnableStateAll(bool state) override;
        void SetCountAll(int count) override;
        void SetVoterLimiting(bool limit) override;

        void ResetVotedList() override;

    private:
        ChatPlayImpl* m_chatplay;

        const AZStd::string m_name;
        AZStd::map<AZStd::string, VoteOption> m_options;
        AZStd::weak_ptr<ChatChannel> m_channel;
        AZStd::map<AZStd::string, CallbackToken> m_callbacks;

        AZStd::mutex m_optionLock;

        bool m_voterLimiting;
        AZStd::set<AZStd::string> m_votedList;

        void OnKeywordSignal(const AZStd::string& option, const AZStd::string& match, const AZStd::string& username);

        void RegisterOptions();
        void UnregisterOptions();
    };

    /******************************************************************************/
    // Implementation of ChatPlayVoteManager
    //

    class ChatPlayVoteManagerImpl
        : public ChatPlayVoteManager
    {
    public:
        explicit ChatPlayVoteManagerImpl(ChatPlayImpl* chatplay);
        ~ChatPlayVoteManagerImpl() override = default;

        AZStd::weak_ptr<ChatPlayVote> GetVote(const AZStd::string& voteId) override;
        void DestroyVote(const AZStd::string& voteId) override;

    private:
        AZStd::map<AZStd::string, AZStd::shared_ptr<ChatPlayVote>> m_votes;

        AZStd::mutex m_votesLock;

        ChatPlayImpl* m_chatplay;
    };


    /******************************************************************************/
    // Helper class for sending one-shot whispers
    //

    class Whisperer
        : public AZStd::enable_shared_from_this < Whisperer >
    {
    public:
        Whisperer(const AZStd::weak_ptr<ChatPlayImpl>& chatPlay,
            const AZStd::string& sender, const AZStd::string& recipient, const AZStd::string& message,
            const WhisperCallback& callback);

        void CreateStream();

    private:
        void OnStreamCreate(dyad::CDyadStream&);
        void OnStreamEvent(dyad::CDyadEvent&);

        AZStd::string MakeGroupServerListURL();

        void QueueCallback(WhisperResult result);

        void ProcessHostList(const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode);
        bool PopulateHostInfoList(HostInfoList& hostInfoList, const Aws::Utils::Json::JsonView& jsonValue, bool isWebsocket);

        // Reference back to ChatPlay instance
        AZStd::weak_ptr<ChatPlayImpl> m_chatPlay;

        // List of group IRC server hosts and ports
        HostInfoList m_hostInfoList;

        // Index of host we are connected to
        int m_connectedHostIndex;

        // Bool to keep track of if we managed a successful connection
        bool m_successfulConnection;

        // State
        bool m_queuedCallback;

        // Message stream handler (IRC or WebSocket)
        AZStd::unique_ptr<IStreamHandler> m_streamHandler;

        // Parameters
        AZStd::string m_sender;
        AZStd::string m_oauthToken;
        AZStd::string m_recipient;
        AZStd::string m_message;

        // Callback
        WhisperCallback m_callback;
    };

    /******************************************************************************/
    CallbackToken ChatChannelImpl::ms_callbackToken = 0;

    ChatChannelImpl::ChatChannelImpl(const AZStd::string& channelId, ChatPlayImpl* chatPlay)
        : m_channelId(channelId)
        , m_chatPlay(chatPlay)
        , m_connectionState(ConnectionState::Disconnected)
        , m_epoch(dyad::InvalidStreamId)
        , m_connectedHostIndex(-1)
        , m_successfulConnection(false)
    {
        ChatPlayChannelRequestBus::Handler::BusConnect(m_channelId);
    }

    ChatChannelImpl::~ChatChannelImpl()
    {
        ChatPlayChannelRequestBus::Handler::BusDisconnect(m_channelId);

        // Disconnect is idempotent, so we can call this unconditionally
        Disconnect();
    }

    const AZStd::string& ChatChannelImpl::GetChannelId() const
    {
        return m_channelId;
    }

    void ChatChannelImpl::Connect()
    {
        /* DISPATCH EVENT THREAD */
        switch (m_connectionState)
        {
        case ConnectionState::Connected:
        case ConnectionState::Connecting:
            // Connection already established or in progress; do nothing
            break;

        case ConnectionState::Disconnected:
        case ConnectionState::Error:
        {
            // Prepare the API request for the list of IRC servers
            AZStd::string requestUrl = MakeServerListURL().c_str();

            HttpRequestor::Callback cb = [=](const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode)
            {
                /* HTTP REQUEST MANAGER THREAD */
                ProcessHostList(jsonValue, responseCode);

                // Create a weak reference for later in case the channel is discarded
                // while the stream is still open
                auto weak = AZStd::weak_ptr<ChatChannelImpl>(shared_from_this());

                // Create a handler that will dispatch the event if the channel is still live
                auto eventHandler = [=](dyad::CDyadEvent& event)
                {
                    if (auto shared = weak.lock())
                    {
                        shared->OnStreamEvent(event);
                    }
                };

                // Create a handler that will dispatch the event if the channel is still live
                auto createHandler = [=](dyad::CDyadStream& stream)
                {
                    if (auto shared = weak.lock())
                    {
                        shared->OnStreamCreate(stream);
                    }
                };

                // Request a stream from Dyad and update the connection state
                m_epoch = m_chatPlay->GetDyad()->CreateStream(eventHandler, createHandler);
            };

            HttpRequestor::Headers headers;
            headers["Client-ID"] = m_chatPlay->GetVars()->GetClientID();

            EBUS_EVENT(HttpRequestor::HttpRequestorRequestBus, AddRequestWithHeaders, requestUrl, Aws::Http::HttpMethod::HTTP_GET, headers, cb);

            ChangeConnectionState(m_epoch, ConnectionState::Connecting);
            break;
        }
        }
    }

    void ChatChannelImpl::Disconnect()
    {
        m_chatPlay->GetDyad()->CloseStream(m_epoch);
    }

    ConnectionState ChatChannelImpl::GetConnectionState()
    {
        return m_connectionState;
    }

    CallbackToken ChatChannelImpl::RegisterConnectionStateChange(const StateCallback& callback)
    {
        CallbackToken token = ++ms_callbackToken;
        m_stateCallbacks.emplace(token, callback);
        return token;
    }

    void ChatChannelImpl::UnregisterConnectionStateChange(CallbackToken token)
    {
        m_stateCallbacks.erase(token);
    }

    CallbackToken ChatChannelImpl::RegisterKeyword(const AZStd::string& keyword, const KeywordCallback& callback)
    {
        CallbackToken token = ++ms_callbackToken;
        m_keywordCallbacks.emplace(token, callback);
        m_tokenToKeyword.emplace(token, keyword);

        m_keywordTokens.emplace(keyword, token);

        if (m_keywordTokens.count(keyword) == 1)
        {
            // If the count is 1, then the keyword must have been just added.
            // Therefore update the synchronized map with the new keyword
            AZStd::regex re(keyword.c_str(), AZStd::regex_constants::icase | AZStd::regex_constants::optimize);
            AZStd::lock_guard<AZStd::mutex> lock(m_keywordLock);
            m_keywords.emplace(keyword, AZStd::move(re));
        }

        return token;
    }

    void ChatChannelImpl::UnregisterKeyword(CallbackToken token)
    {
        auto i = m_tokenToKeyword.find(token);
        if (i != m_tokenToKeyword.end())
        {
            const AZStd::string& keyword = i->second;

            auto lower = m_keywordTokens.lower_bound(keyword);
            auto upper = m_keywordTokens.upper_bound(keyword);
            for (auto j = lower; j != upper; ++j)
            {
                if (j->second == token)
                {
                    m_keywordTokens.erase(j);
                    break;
                }
            }

            if (!m_keywordTokens.count(keyword))
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_keywordLock);
                m_keywords.erase(keyword);
            }
        }

        m_keywordCallbacks.erase(token);
        m_tokenToKeyword.erase(token);
    }

    void ChatChannelImpl::PostConnectionState(dyad::StreamId epoch, ConnectionState state)
    {
        /* ANY THREAD */
        m_chatPlay->RegisterEvent(AZStd::bind(&ChatChannelImpl::ChangeConnectionState, this, epoch, state));
    }

    void ChatChannelImpl::ChangeConnectionState(dyad::StreamId epoch, ConnectionState state)
    {
        /* DISPATCH EVENT THREAD */
        if (epoch != m_epoch)
        {
            // Discard old events
            return;
        }

        if (state == ConnectionState::Disconnected && m_connectionState == ConnectionState::Error)
        {
            // Error state persists until connection
            return;
        }

        m_connectionState = state;

        ChatPlayChannelNotificationBus::Event(m_channelId, &ChatPlayChannelNotificationBus::Events::OnConnectionStateChanged, m_connectionState);

        if (!m_stateCallbacks.empty())
        {
            // Extracting the tokens to a local map prevents modifications to the map during one
            // of the callbacks from invalidating our iterator
            AZStd::vector<CallbackToken> tokens;
            tokens.reserve(m_stateCallbacks.size());
            for (const auto& callback : m_stateCallbacks)
            {
                tokens.push_back(callback.first);
            }

            // Loop through tokens
            for (CallbackToken t : tokens)
            {
                // Check that the token is still valid (i.e. that another callback didn't remove it)
                auto it = m_stateCallbacks.find(t);
                if (it != m_stateCallbacks.end())
                {
                    if (it->second)
                    {
                        it->second(m_connectionState);
                    }
                }
            }
        }
    }

    void ChatChannelImpl::OnChatbotReceived(dyad::StreamId epoch, const AZStd::string& msg)
    {
        /*DYAD THREAD*/

        // message = [":" prefix SPACE] command [SPACE params] crlf
        AZStd::string prefix;
        AZStd::string command;
        AZStd::string::size_type marker = 0;
        if (!msg.empty() && msg[0] == ':')
        {
            marker = msg.find(' ');
            prefix = msg.substr(1, marker - 1);
        }
        if (marker != AZStd::string::npos)
        {
            // +1 to skip ' '
            auto start = marker + 1;
            marker = msg.find(' ', start);
            command = msg.substr(start, marker - start);
        }
        if (command == "PRIVMSG")
        {
            // params: <msgtarget> SPACE <text to be sent>
            AZStd::string target;
            AZStd::string params;
            if (marker != AZStd::string::npos)
            {
                // +1 to skip ' '
                auto start = marker + 1;
                marker = msg.find(' ', start);
                target = msg.substr(start, marker - start);
            }
            if (marker != AZStd::string::npos)
            {
                // +2 to skip " :"
                if (msg.size() > marker + 2)
                {
                    params = msg.substr(marker + 2); // message

                    // :sender!sender@sender.tmi.twitch.tv PRIVMSG #recipient :hello

                    marker = prefix.find('!');
                    AZStd::string username = prefix.substr(0, marker);

                    AZStd::lock_guard<AZStd::mutex> lock(m_keywordLock);
                    for (auto& kvp : m_keywords)
                    {
                        const AZStd::string& keyword = kvp.first;
                        const AZStd::regex& regex = kvp.second;
                        AZStd::cmatch match;
                        if (AZStd::regex_search(params.c_str(), match, regex))
                        {
                            AZStd::string matched(AZStd::string(match[0]).c_str());
                            m_chatPlay->RegisterEvent(AZStd::bind(&ChatChannelImpl::KeywordEvent, this, epoch, keyword, matched, username));
                        }
                    }
                }
            }
        }
    }

    void ChatChannelImpl::KeywordEvent(dyad::StreamId epoch, const AZStd::string& keyword, const AZStd::string& match, const AZStd::string& username)
    {
        /*DISPATCH EVENT THREAD*/
        if (epoch != m_epoch)
        {
            // Discard old events
            return;
        }

        ChatPlayChannelNotificationBus::Event(m_channelId, &ChatPlayChannelNotificationBus::Events::OnKeywordMatched, keyword, match, username);

        auto lower = m_keywordTokens.lower_bound(keyword);
        auto upper = m_keywordTokens.upper_bound(keyword);
        for (auto i = lower; i != upper; ++i)
        {
            auto callback = m_keywordCallbacks[i->second];
            if (callback)
            {
                callback(match, username);
            }
        }
    }

    void ChatChannelImpl::OnStreamCreate(dyad::CDyadStream& stream)
    {
        /* DYAD THREAD */

        if (stream.GetId() != m_epoch)
        {
            // Discard old events
            return;
        }

        bool connected = false;

        for (int i = 0; i < m_hostInfoList.size(); i++)
        {
            HostInfo& hostInfo = m_hostInfoList[i];

            if (hostInfo.connectionFailed) {
                // This hostinfo failed already during this connection attempt, try next one
                continue;
            }

            AZ_TracePrintf("ChatPlay", "Connecting to %s:%d (%s)...", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");

            if (stream.Connect(hostInfo.address.c_str(), hostInfo.port))
            {
                m_connectedHostIndex = i;

                dyad::StreamId id = stream.GetId();
                AZStd::weak_ptr<ChatChannelImpl> channel = shared_from_this();

                auto messageCallback = AZStd::bind(&ChatChannelImpl::OnChatbotReceived, this, id, AZStd::placeholders::_1);

                auto rawSend = [=](const char* message, size_t messageLength)
                {
                    if (auto ptr = channel.lock())
                    {
                        if (auto chatPlay = ptr->m_chatPlay)
                        {
                            AZStd::string toCopy(message, messageLength);
                            chatPlay->GetDyad()->PostStreamAction(id, [=](dyad::CDyadStream& stream){
                                stream.Write(toCopy.c_str(), toCopy.size());
                            });
                        }
                    }
                };

                if (hostInfo.websocket)
                {
                    // Make IRC handler and use WebSocket handler as translation layer
                    auto ircHandler = AZStd::make_shared<IRCStream>(m_chatPlay->GetVars()->GetUser(), m_chatPlay->GetVars()->GetPassword(), m_channelId.c_str());

                    m_streamHandler = AZStd::unique_ptr<WebSocketStream>(new WebSocketStream(hostInfo.address.c_str(), ircHandler));
                    m_streamHandler->SetSendFunction(rawSend);

                    ircHandler->SetMessageFunction(messageCallback);

                }
                else
                {
                    // Make IRC handler only
                    auto ircStream = AZStd::unique_ptr<IRCStream>(new IRCStream(m_chatPlay->GetVars()->GetUser(), m_chatPlay->GetVars()->GetPassword(), m_channelId.c_str()));
                    ircStream->SetMessageFunction(messageCallback);

                    m_streamHandler = AZStd::move(ircStream);
                    m_streamHandler->SetSendFunction(rawSend);
                }

                connected = true;
                break;
            }
            else
            {
                // Flag connection as failed
                hostInfo.connectionFailed = true;
                AZ_Warning("ChatPlay", false, "Failed to connect to %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");
            }
        }

        // If connected is still false here, all connections failed
        if (!connected)
        {
            AZ_Warning("ChatPlay", false, "Failed to connect to the chat server for the channel \"%s\": all connection configurations failed.", m_channelId.c_str());
            PostConnectionState(m_epoch, ConnectionState::Error);

            // Reset HostInfo flags
            ChatPlayCVars::ResetHostInfoFlags(m_hostInfoList);

            // Reset connected host
            m_connectedHostIndex = -1;
        }
    }

    void ChatChannelImpl::ProcessHostList(const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode)
    {
        /* HTTP REQUEST MANAGER THREAD */

        if (static_cast<Aws::Http::HttpResponseCode>(responseCode) == Aws::Http::HttpResponseCode::OK)
        {
            HostInfoList hostInfoList;

            // add IRC hosts to list
            if (!PopulateHostInfoList(hostInfoList, jsonValue, false))
            {
                AZ_Warning("ChatPlay", false, "Error parsing IRC host list for the channel \"%s\".", m_channelId.c_str());
                PostConnectionState(m_epoch, ConnectionState::Error);
            }

            // add websocket hosts to list
            if (!PopulateHostInfoList(hostInfoList, jsonValue, true))
            {
                AZ_Warning("ChatPlay", false, "Error parsing IRC websocket host list for the channel \"%s\".", m_channelId.c_str());
                PostConnectionState(m_epoch, ConnectionState::Error);
            }

            AZStd::sort(hostInfoList.begin(), hostInfoList.end(), [](const HostInfo& a, const HostInfo& b)
            {
                return a.priority < b.priority;
            });

            m_hostInfoList.swap(hostInfoList);
        }
        else
        {
            // Could not get list of chat server IPs
            // TODO: handle this better
            AZ_Warning("ChatPlay", false, "Error retrieving IRC host list for the channel \"%s\".", m_channelId.c_str());
            PostConnectionState(m_epoch, ConnectionState::Error);
        }
    }

    bool ChatChannelImpl::PopulateHostInfoList(HostInfoList& hostInfoList, const Aws::Utils::Json::JsonView& jsonValue, bool isWebsocket)
    {
        const char* jsonNodeName = isWebsocket ? "websockets_servers" : "servers";
        auto serverList = jsonValue.GetArray(jsonNodeName);

        for (int i = 0; i < serverList.GetLength(); i++)
        {
            AZStd::string oneHostAndOnePort(serverList.GetItem(i).AsString().c_str());
            AZStd::size_t portStartPosition = oneHostAndOnePort.find(':');
            if (portStartPosition == AZStd::string::npos)
            {
                // Returned data is not what we expected
                return false;
            }
            HostInfo hostInfo;
            hostInfo.address = oneHostAndOnePort.substr(0, portStartPosition);
            AZStd::string stringPort = oneHostAndOnePort.substr(portStartPosition + 1);
            hostInfo.port = ::atoi(stringPort.c_str());
            hostInfo.websocket = isWebsocket;
            hostInfo.ssl = m_chatPlay->GetVars()->IsPortSSL(hostInfo.port, isWebsocket);
            hostInfo.priority = m_chatPlay->GetVars()->GetPortPriority(hostInfo.port, isWebsocket);

            if (hostInfo.IsValid())
            {
                hostInfoList.push_back(hostInfo);
            }
        }

        return true;
    }

    void ChatChannelImpl::OnStreamEvent(dyad::CDyadEvent& event)
    {
        /* DYAD THREAD */
        dyad::StreamId epoch = event.GetStream().GetId();
        if (epoch != m_epoch)
        {
            // Discard old events
            return;
        }

        switch (event.GetType())
        {
        case dyad::EventType::Accept: // we don't accept
        case dyad::EventType::Listen: // we don't listen
            break;

        case dyad::EventType::Tick: // no action needed
        case dyad::EventType::Timeout: // no action needed (timeout leads to closure)
            break;

        case dyad::EventType::Close:
            if (m_successfulConnection)
            {
                PostConnectionState(epoch, ConnectionState::Disconnected);

                // Reset so we try all hosts again next time we try to connect

                // Reset HostInfo flags
                ChatPlayCVars::ResetHostInfoFlags(m_hostInfoList);

                // Reset connected host
                m_connectedHostIndex = -1;

                // Reset successful connection
                m_successfulConnection = false;
            }
            else
            {
                HostInfo& hostInfo = m_hostInfoList[m_connectedHostIndex];
                AZ_Warning("ChatPlay", false, "Failed to connect to %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");

                // Flag current connection as unsuccessful
                hostInfo.connectionFailed = true;

                // Try next connection
                auto stream = event.GetStream();
                OnStreamCreate(stream);
            }
            break;

        case dyad::EventType::Connect:
        {
            auto handlerState = m_streamHandler->OnConnect();
            if (handlerState == IStreamHandler::HandlerState::HANDLER_ERROR)
            {
                // Handler encountered an error
                PostConnectionState(epoch, ConnectionState::Error);
                event.GetStream().Close();
            }
            break;
        }
        case dyad::EventType::Line:
            //Enable if debugging; but not really helpful for everyone in production ...
            //CryLog("%s\n", event.GetData());
            break;

        case dyad::EventType::Error:
            if (m_successfulConnection)
            {
                PostConnectionState(epoch, ConnectionState::Error);
            }
            break;

        case dyad::EventType::Destroy:
            break;

        case dyad::EventType::Data:
        {
            auto handlerState = m_streamHandler->OnMessage(event.GetData(), event.GetDataLength());
            if (handlerState == IStreamHandler::HandlerState::HANDLER_ERROR)
            {
                // Handler encountered an error
                PostConnectionState(epoch, ConnectionState::Error);
                event.GetStream().Close();
            }
            else if (handlerState == IStreamHandler::HandlerState::CONNECTED)
            {
                // Successfully connected
                PostConnectionState(epoch, ConnectionState::Connected);
                m_successfulConnection = true;

                const HostInfo& hostInfo = m_hostInfoList[m_connectedHostIndex];
                AZ_TracePrintf("ChatPlay", "Successfully connected to %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");
            }

            break;
        }
        case dyad::EventType::Ready:
            break;
        }
    }

    AZStd::string ChatChannelImpl::MakeServerListURL()
    {
        AZStd::string requestUrl = AZStd::string::format("https://%s/servers?channel=%s", m_chatPlay->GetVars()->GetAPIServerAddress(), m_channelId.c_str());
        return requestUrl;
    }



    /******************************************************************************/

    ChatPlayImpl::ChatPlayImpl()
    {
        m_dyad = dyad::IDyad::GetInstance();
        AZ_Assert(m_dyad, "ChatPlay was unable to get the Dyad instance"); // This is unexpected, GetInstance should always work; we assert since exceptions are disabled ...

        m_vars = ChatPlayCVars::GetInstance();
        AZ_Assert(m_vars, "ChatPlay was unable to get the CVars instance"); // This is unexpected, GetInstance should always work; we assert since exceptions are disabled ...

        m_voteManager = AZStd::make_unique<ChatPlayVoteManagerImpl>(this);
    }

    ChatPlayImpl::~ChatPlayImpl()
    {
        DispatchEvents();

        // Very bad; references still exist!
        // (*this) is still in use for callbacks ...
        CRY_ASSERT(m_channelMap.empty());
    }

    AZStd::weak_ptr<ChatChannel> ChatPlayImpl::GetChatChannel(const AZStd::string& _channelId)
    {
        AZStd::shared_ptr<ChatChannel> ptr;

        AZStd::string channelId = _channelId;
        AZStd::to_lower(channelId.begin(), channelId.end());

        // Ensure that the map contains an entry for this channel
        auto entry = m_channelMap.find(channelId);
        if (entry != m_channelMap.end())
        {
            // Map entry found; attempt to get the shared_ptr
            ptr = entry->second;
        }
        else
        {
            // Map entry not found; create one and assign a nullptr
            AZStd::lock_guard<AZStd::mutex> lock(m_channelLock);
            auto i = m_channelMap.emplace(channelId, AZStd::shared_ptr<ChatChannel>());

            if (!i.second)
            {
                // C++ runtime error; element not found and not insertable!?
                CRY_ASSERT(i.second);
                return AZStd::weak_ptr<ChatChannel>();
            }

            entry = i.first;
        }

        // Ensure that whatever is assigned to the entry is valid; create a new channel if not
        if (!ptr)
        {
            ptr = AZStd::make_shared<ChatChannelImpl>(channelId, this);
            entry->second = ptr;
        }

        return ptr;
    }

    void ChatPlayImpl::DestroyChatChannel(const AZStd::string& _channelId)
    {
        AZStd::string channelId = _channelId;
        AZStd::to_lower(channelId.begin(), channelId.end());

        // Try and find the entry for this channel
        auto entry = m_channelMap.find(channelId);
        if (entry != m_channelMap.end())
        {
            // Disconnect and remove the channel
            entry->second->Disconnect();

            AZStd::lock_guard<AZStd::mutex> lock(m_channelLock);
            m_channelMap.erase(entry);
        }
    }

    void ChatPlayImpl::DisconnectAll()
    {
        // iterate through map of ChatChannels and disconnect them all.
        for (auto i : m_channelMap)
        {
            if (auto ptr = i.second)
            {
                ptr->Disconnect();
            }
        }
    }

    size_t ChatPlayImpl::DispatchEvents()
    {
        AZStd::vector<ChatPlayEvent> events;

        {
            AZStd::lock_guard<AZStd::mutex> lock(m_eventLock);
            AZStd::swap(m_events, events);
        }

        for (const auto& event : events)
        {
            if (event)
            {
                event();
            }
        }

        // Cleans up after dead channels
        //
        // TODO: Make the channel post an event to ChatPlay on destruction to
        // avoid needing to loop over everything...
        for (auto iter = m_channelMap.begin(); iter != m_channelMap.end();)
        {
            if (iter->second)
            {
                ++iter;
            }
            else
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_channelLock);
                iter = m_channelMap.erase(iter);
            }
        }

        return events.size();
    }

    void ChatPlayImpl::RegisterEvent(const ChatPlayEvent& event)
    {
        /*ANY THREAD*/

        AZStd::lock_guard<AZStd::mutex> lock(m_eventLock);
        m_events.push_back(event);
    }

    const AZStd::shared_ptr<ChatPlayCVars>& ChatPlayImpl::GetVars()
    {
        return m_vars;
    }

    const std::shared_ptr<dyad::IDyad>& ChatPlayImpl::GetDyad()
    {
        return m_dyad;
    }

    AZStd::shared_ptr<ChatPlay> ChatPlay::CreateInstance()
    {
        AZStd::shared_ptr<ChatPlay> chatplay = AZStd::static_pointer_cast<ChatPlay>(AZStd::make_shared<ChatPlayImpl>());
        return chatplay;
    }

    void ChatPlayImpl::RegisterCredentials(const AZStd::string& _username, const AZStd::string& oauthToken)
    {
        AZStd::string username = _username;
        AZStd::to_lower(username.begin(), username.end());

        m_credentialMap[username] = oauthToken;
    }

    void ChatPlayImpl::UnregisterCredentials(const AZStd::string& _username)
    {
        AZStd::string username = _username;
        AZStd::to_lower(username.begin(), username.end());

        m_credentialMap.erase(username);
    }

    void ChatPlayImpl::UnregisterAllCredentials()
    {
        m_credentialMap.clear();
    }

    const char* ChatPlayImpl::GetOAuthToken(const AZStd::string& _username)
    {
        AZStd::string username = _username;
        AZStd::to_lower(username.begin(), username.end());

        auto token = m_credentialMap.find(username);

        if (token != m_credentialMap.end())
        {
            return token->second.c_str();
        }

        // no matching token found
        return nullptr;
    }

    void ChatPlayImpl::SendWhisper(const AZStd::string& _sender, const AZStd::string& _recipient, const AZStd::string& message,
        const WhisperCallback& callback)
    {
        AZStd::string sender = _sender;
        AZStd::to_lower(sender.begin(), sender.end());

        AZStd::string recipient = _recipient;
        AZStd::to_lower(recipient.begin(), recipient.end());

        AZStd::make_shared<Whisperer>(shared_from_this(), sender, recipient, message, callback)->CreateStream();
    }

    ChatPlayVoteManager* ChatPlayImpl::GetVoteManager() const
    {
        return m_voteManager.get();
    }

    /******************************************************************************/

    ChatPlayVoteImpl::ChatPlayVoteImpl(const AZStd::string& name, ChatPlayImpl* chatplay)
        : m_name(name)
        , m_chatplay(chatplay)
        , m_voterLimiting(false)
    {
        ChatPlayVoteRequestBus::Handler::BusConnect(m_name);
    }

    ChatPlayVoteImpl::~ChatPlayVoteImpl()
    {
        ChatPlayVoteRequestBus::Handler::BusDisconnect(m_name);
        ClearChannel();
    }

    const AZStd::string& ChatPlayVoteImpl::GetName() const
    {
        return m_name;
    }

    bool ChatPlayVoteImpl::AddOption(const AZStd::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        if (m_options.find(name) == m_options.end())
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_optionLock);
            m_options.emplace(name, VoteOption(name));
            RegisterOptions();
            return true;
        }

        // The option probably already exists
        return false;
    }

    void ChatPlayVoteImpl::RemoveOption(const AZStd::string& name)
    {
        const auto& registered = m_options.find(name);
        if (registered != m_options.end())
        {
            // Unregister the option from the callback to be safe
            auto it = m_callbacks.find(name);

            // Try and lock the channel
            auto channel = m_channel.lock();

            if (channel && it != m_callbacks.end())
            {
                channel->UnregisterKeyword(it->second);
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_optionLock);
            m_options.erase(registered);
        }
    }

    void ChatPlayVoteImpl::ConfigureOption(const AZStd::string& optionName, int count, bool enabled)
    {
        auto it = m_options.find(optionName);
        if (it != m_options.end())
        {
            VoteOption& option = it->second;
            option.SetCount(count);
            option.SetEnabled(enabled);
        }
    }

    bool ChatPlayVoteImpl::OptionExists(const AZStd::string& name)
    {
        return m_options.find(name) != m_options.end();
    }

    int ChatPlayVoteImpl::GetOptionCount(const AZStd::string& optionName)
    {
        auto it = m_options.find(optionName);
        if (it != m_options.end())
        {
            const VoteOption& option = it->second;
            return option.GetCount();
        }

        return 0;
    }

    void ChatPlayVoteImpl::SetOptionCount(const AZStd::string& optionName, int count)
    {
        auto it = m_options.find(optionName);
        if (it != m_options.end())
        {
            VoteOption& option = it->second;
            option.SetCount(count);
        }
    }

    bool ChatPlayVoteImpl::GetOptionEnabled(const AZStd::string& optionName)
    {
        auto it = m_options.find(optionName);
        if (it != m_options.end())
        {
            const VoteOption& option = it->second;
            return option.GetEnabled();
        }

        return false;
    }

    void ChatPlayVoteImpl::SetOptionEnabled(const AZStd::string& optionName, bool enabled)
    {
        auto it = m_options.find(optionName);
        if (it != m_options.end())
        {
            VoteOption& option = it->second;
            option.SetEnabled(enabled);
        }
    }

    bool ChatPlayVoteImpl::SetChannel(const AZStd::string& _name)
    {
        AZStd::string name = _name;
        AZStd::to_lower(name.begin(), name.end());

        auto channel = m_channel.lock();
        if (channel && channel->GetChannelId() != name.c_str())
        {
            ClearChannel();
        }

        if (!name.empty())
        {
            // Check if ChatPlay is available and enabled
            if (m_chatplay)
            {
                m_channel = m_chatplay->GetChatChannel(name.c_str());
                RegisterOptions();
            }
        }
        return !m_channel.expired();    ///< Fix for bad logic when determining the return value of SetChannel.
    }

    void ChatPlayVoteImpl::Visit(const AZStd::function<void(VoteOption& option)>& visitor)
    {
        for (auto& registered : m_options)
        {
            visitor(registered.second);
        }
    }

    void ChatPlayVoteImpl::SetEnableStateAll(bool state)
    {
        Visit([state](VoteOption& option){
            option.SetEnabled(state);
        });
    }

    void ChatPlayVoteImpl::SetCountAll(int count)
    {
        Visit([count](VoteOption& option){
            option.SetCount(count);
        });
    }

    void ChatPlayVoteImpl::SetVoterLimiting(bool limiting)
    {
        m_voterLimiting = limiting;
    }

    void ChatPlayVoteImpl::ResetVotedList()
    {
        m_votedList.clear();
    }

    void ChatPlayVoteImpl::ClearChannel()
    {
        UnregisterOptions();
        m_channel.reset();
    }

    void ChatPlayVoteImpl::OnKeywordSignal(const AZStd::string& option, const AZStd::string& /*match*/, const AZStd::string& _username)
    {
        const auto& registered = m_options.find(option);
        if (registered != m_options.end())
        {
            VoteOption& option = registered->second;
            if (option.GetEnabled())
            {
                if (m_voterLimiting)
                {
                    AZStd::string username = _username;
                    AZStd::to_lower(username.begin(), username.end());

                    auto voter = m_votedList.find(username);
                    if (voter != m_votedList.end())
                    {
                        // This user has already voted
                        return;
                    }
                    else
                    {
                        // Add user to voted list and proceed to increase vote count
                        m_votedList.insert(username);
                    }
                }

                // Assume one count per signal ...
                option.SetCount(option.GetCount() + 1);
            }
        }
    }

    void ChatPlayVoteImpl::RegisterOptions()
    {
        if (auto channel = m_channel.lock())
        {
            for (auto& option : m_options)
            {
                const AZStd::string& optionName = option.first;
                if (m_callbacks.find(optionName) != m_callbacks.end())
                {
                    continue;
                }

                auto callback = AZStd::bind(&ChatPlayVoteImpl::OnKeywordSignal, this, optionName, AZStd::placeholders::_1, AZStd::placeholders::_2);
                auto token = channel->RegisterKeyword(optionName.c_str(), AZStd::move(callback));

                m_callbacks[optionName] = token;
            }
        }
    }

    void ChatPlayVoteImpl::UnregisterOptions()
    {
        if (auto channel = m_channel.lock())
        {
            for (auto& registered : m_callbacks)
            {
                channel->UnregisterKeyword(registered.second);
            }
        }
        m_callbacks.clear();
    }

    /******************************************************************************/

    ChatPlayVoteManagerImpl::ChatPlayVoteManagerImpl(ChatPlayImpl* chatplay)
        : m_chatplay(chatplay)
    {
    }

    AZStd::weak_ptr<ChatPlayVote> ChatPlayVoteManagerImpl::GetVote(const AZStd::string& voteId)
    {
        AZStd::shared_ptr<ChatPlayVote> ptr;

        // Ensure that the map contains an entry for this channel
        auto entry = m_votes.find(voteId);
        if (entry != m_votes.end())
        {
            // Map entry found; attempt to get the shared_ptr
            ptr = entry->second;
        }
        else
        {
            // Map entry not found; create one and assign a nullptr
            AZStd::lock_guard<AZStd::mutex> lock(m_votesLock);
            auto i = m_votes.emplace(voteId, AZStd::shared_ptr<ChatPlayVote>());

            if (!i.second)
            {
                // C++ runtime error; element not found and not insertable!?
                CRY_ASSERT(i.second);
                return AZStd::weak_ptr<ChatPlayVote>();
            }

            entry = i.first;
        }

        // Ensure that whatever is assigned to the entry is valid; create a new vote if not
        if (!ptr)
        {
            ptr = AZStd::make_shared<ChatPlayVoteImpl>(voteId, m_chatplay);
            entry->second = ptr;
        }

        return ptr;
    }

    void ChatPlayVoteManagerImpl::DestroyVote(const AZStd::string& voteId)
    {
        // Try and find the entry for this vote
        auto entry = m_votes.find(voteId);
        if (entry != m_votes.end())
        {
            // Removing the entry will cause the vote to destroy
            AZStd::lock_guard<AZStd::mutex> lock(m_votesLock);
            m_votes.erase(entry);
        }
    }

    /******************************************************************************/

    Whisperer::Whisperer(const AZStd::weak_ptr<ChatPlayImpl>& chatPlay,
        const AZStd::string& sender, const AZStd::string& recipient, const AZStd::string& message,
        const WhisperCallback& callback)
        : m_chatPlay(chatPlay)
        , m_sender(sender)
        , m_connectedHostIndex(-1)
        , m_successfulConnection(false)
        , m_queuedCallback(false)
        , m_recipient(recipient)
        , m_message(message)
        , m_callback(callback)
    {
    }

    void Whisperer::CreateStream()
    {
        AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
        if (!chatPlay)
        {
            return;
        }

        m_oauthToken = chatPlay->GetOAuthToken(m_sender);

        if (m_oauthToken.empty())
        {
            // No registered oauth token, queue error callback and return
            QueueCallback(WhisperResult::MissingOAuthToken);
            return;
        }

        // Prepare the API request for the list of IRC servers
        AZStd::string requestUrl = MakeGroupServerListURL().c_str();

        // Note: shared_from_this() can't be used in the class constructor
        AZStd::shared_ptr<Whisperer> whisperer = shared_from_this();

        HttpRequestor::Callback cb = [=](const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode) {

            /* HTTP REQUEST MANAGER THREAD */
            AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
            if (!chatPlay)
            {
                return;
            }

            ProcessHostList(jsonValue, responseCode);

            // Create a handler that will dispatch the event if the channel is still live
            auto eventHandler = [=](dyad::CDyadEvent& event){
                whisperer->OnStreamEvent(event);
            };

            // Create a handler that will dispatch the event if the channel is still live
            auto createHandler = [=](dyad::CDyadStream& stream) {
                whisperer->OnStreamCreate(stream);
            };

            // Request a stream from Dyad
            chatPlay->GetDyad()->CreateStream(eventHandler, createHandler);
        };

        EBUS_EVENT(HttpRequestor::HttpRequestorRequestBus, AddRequest, requestUrl, Aws::Http::HttpMethod::HTTP_GET, cb);
    }

    void Whisperer::OnStreamCreate(dyad::CDyadStream& stream)
    {
        /* DYAD THREAD */

        AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
        if (!chatPlay)
        {
            stream.Close();
            return;
        }

        bool connected = false;

        for (int i = 0; i < m_hostInfoList.size(); i++)
        {
            HostInfo& hostInfo = m_hostInfoList[i];

            if (hostInfo.connectionFailed) {
                // This hostinfo failed already during this connection attempt, try next one
                continue;
            }

            AZ_TracePrintf("Whisper", "Connecting to %s:%d (%s)...", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");

            if (stream.Connect(hostInfo.address.c_str(), hostInfo.port))
            {
                m_connectedHostIndex = i;

                dyad::StreamId id = stream.GetId();
                AZStd::weak_ptr<Whisperer> whisperer = shared_from_this();

                auto rawSend = [=](const char* message, size_t messageLength)
                {
                    if (auto ptr = whisperer.lock())
                    {
                        if (auto chatPlay = ptr->m_chatPlay.lock())
                        {
                            AZStd::string toCopy(message, messageLength);
                            chatPlay->GetDyad()->PostStreamAction(id, [=](dyad::CDyadStream& stream){
                                stream.Write(toCopy.c_str(), toCopy.size());
                            });
                        }
                    }
                };

                if (hostInfo.websocket)
                {
                    auto ircHandler = AZStd::make_shared<IRCStream>(m_sender.c_str(), m_oauthToken.c_str(), nullptr);

                    m_streamHandler = AZStd::unique_ptr<WebSocketStream>(new WebSocketStream(hostInfo.address.c_str(), ircHandler));
                    m_streamHandler->SetSendFunction(rawSend);
                }
                else
                {
                    m_streamHandler = AZStd::unique_ptr<IRCStream>(new IRCStream(m_sender.c_str(), m_oauthToken.c_str(), nullptr));
                    m_streamHandler->SetSendFunction(rawSend);
                }

                //CryLog("[ChatPlay] Connecting to %s:%d (%s)", m_connectedHost.address.c_str(), m_connectedHost.port, m_connectedHost.websocket ? "websocket" : "irc");
                connected = true;
                break;
            }
            else
            {
                hostInfo.connectionFailed = true;
                AZ_Warning("Whisper", false, "Failed to connect to %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");
            }
        }

        // If connected is still false here, all connections failed
        if (!connected && !m_queuedCallback)
        {
            AZ_Warning("Whisper", false, "Failed to connect to the chat server, all connection configurations failed.");

            QueueCallback(WhisperResult::ConnectionError);
            stream.Close();

            // Reset HostInfo flags
            ChatPlayCVars::ResetHostInfoFlags(m_hostInfoList);

            // Reset connected host
            m_connectedHostIndex = -1;
        }
    }

    void Whisperer::OnStreamEvent(dyad::CDyadEvent& event)
    {
        /* DYAD THREAD */
        dyad::StreamId epoch = event.GetStream().GetId();

        switch (event.GetType())
        {
        case dyad::EventType::Accept: // we don't accept
        case dyad::EventType::Listen: // we don't listen
            break;

        case dyad::EventType::Tick: // no action needed
        case dyad::EventType::Timeout: // no action needed (timeout leads to closure)
            break;

        case dyad::EventType::Close:
            if (m_successfulConnection)
            {
                // Reset so we try all hosts again next time we try to connect

                // Reset HostInfo flags
                ChatPlayCVars::ResetHostInfoFlags(m_hostInfoList);

                // Reset connected host
                m_connectedHostIndex = -1;

                // Reset successful connection
                m_successfulConnection = false;
            }
            else
            {
                if (m_connectedHostIndex >= 0 && m_connectedHostIndex < m_hostInfoList.size())
                {
                    HostInfo& hostInfo = m_hostInfoList[m_connectedHostIndex];
                    AZ_Warning("Whisper", false, "Failed to connect to %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");

                    if (!m_queuedCallback)
                    {
                        // Flag current connection as unsuccessful
                        hostInfo.connectionFailed = true;

                        // Try next connection
                        auto stream = event.GetStream();
                        OnStreamCreate(stream);
                    }
                }
                else
                {
                    // Shouldn't happen
                    AZ_Warning("Whisper", false, "A whisper's connected host index was out of bounds");
                    QueueCallback(WhisperResult::ConnectionError);
                    event.GetStream().Close();
                }
            }
            break;

        case dyad::EventType::Connect:
        {
            auto handlerState = m_streamHandler->OnConnect();

            if (handlerState == IStreamHandler::HandlerState::HANDLER_ERROR)
            {
                // Handler encountered an error (null callback?)
                // Close and try next connection method
                event.GetStream().Close();
            }
            break;
        }
        case dyad::EventType::Line:
            //CryLog("%s\n", event.GetData());
            break;

        case dyad::EventType::Error:
            break;

        case dyad::EventType::Destroy:
            if (!m_queuedCallback)
            {
                // Destroyed unexpectedly, queue error callback and close stream
                // Close and try next connection method
                event.GetStream().Close();
            }
            break;

        case dyad::EventType::Data:
        {
            auto handlerState = m_streamHandler->OnMessage(event.GetData(), event.GetDataLength());

            if (handlerState == IStreamHandler::HandlerState::CONNECTED)
            {
                // Successfully connected
                m_successfulConnection = true;

                // Prepare and send the whisper command
                AZStd::string message = AZStd::string::format("PRIVMSG #%s :/w %s %s\r\n", m_recipient.c_str(), m_recipient.c_str(), m_message.c_str());

                if (!m_streamHandler->SendMessage(message.c_str(), message.length()))
                {
                    // Handler was not able to send message
                    QueueCallback(WhisperResult::ConnectionError);
                    event.GetStream().Close();
                }
            }
            else if (handlerState == IStreamHandler::HandlerState::MESSAGE_SENT)
            {
                if (m_connectedHostIndex >= 0 && m_connectedHostIndex < m_hostInfoList.size())
                {
                    // Message sent, queue up the success callback
                    const HostInfo& hostInfo = m_hostInfoList[m_connectedHostIndex];
                    AZ_TracePrintf("Whisper", "Successfully sent whisper on %s:%d (%s)", hostInfo.address.c_str(), hostInfo.port, hostInfo.websocket ? "WebSocket" : "IRC");
                    QueueCallback(WhisperResult::Success);

                    // Done, we can close the stream
                    event.GetStream().Close();
                }
                else
                {
                    // Shouldn't happen
                    AZ_Warning("Whisper", false, "A whisper's connected host index was out of bounds");
                    QueueCallback(WhisperResult::ConnectionError);
                    event.GetStream().Close();
                }

            }
            else if (handlerState == IStreamHandler::HandlerState::HANDLER_ERROR)
            {
                // Handler encountered an error
                // Close, which will trigger next connection attempt
                event.GetStream().Close();
            }
            break;
        }

        case dyad::EventType::Ready:
            break;
        }
    }

    AZStd::string Whisperer::MakeGroupServerListURL()
    {
        AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
        if (!chatPlay)
        {
            return AZStd::string();
        }

        AZStd::string requestUrl = AZStd::string::format("https://%s/servers?cluster=group", chatPlay->GetVars()->GetAPIServerAddress());
        return requestUrl;
    }

    void Whisperer::QueueCallback(WhisperResult result)
    {
        AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
        if (!chatPlay)
        {
            return;
        }

        chatPlay->RegisterEvent(AZStd::bind(m_callback, result));
        m_queuedCallback = true;
    }

    void Whisperer::ProcessHostList(const Aws::Utils::Json::JsonView& jsonValue, Aws::Http::HttpResponseCode responseCode)
    {
        /* HTTP REQUEST MANAGER THREAD */

        if (static_cast<Aws::Http::HttpResponseCode>(responseCode) == Aws::Http::HttpResponseCode::OK)
        {
            HostInfoList hostInfoList;

            // add IRC hosts to list
            if (!PopulateHostInfoList(hostInfoList, jsonValue, false))
            {
                AZ_Warning("Whisper", false, "Error parsing group IRC host list.");
                QueueCallback(WhisperResult::ConnectionError);
            }

            // add websocket hosts to list
            if (!PopulateHostInfoList(hostInfoList, jsonValue, true))
            {
                AZ_Warning("Whisper", false, "Error parsing group IRC websocket host list.");
                QueueCallback(WhisperResult::ConnectionError);
            }

            AZStd::sort(hostInfoList.begin(), hostInfoList.end(), [](const HostInfo& a, const HostInfo& b)
            {
                return a.priority < b.priority;
            });

            m_hostInfoList.swap(hostInfoList);
        }
        else
        {
            // Could not get list of chat server IPs
            // TODO: handle this better
            AZ_Warning("Whisper", false, "Error retrieving group IRC host list.");
            QueueCallback(WhisperResult::ConnectionError);
        }
    }

    bool Whisperer::PopulateHostInfoList(HostInfoList& hostInfoList, const Aws::Utils::Json::JsonView& jsonValue, bool isWebsocket)
    {
        AZStd::shared_ptr<ChatPlayImpl> chatPlay = m_chatPlay.lock();
        if (!chatPlay)
        {
            return false;
        }

        const char* jsonNodeName = isWebsocket ? "websockets_servers" : "servers";
        auto serverList = jsonValue.GetArray(jsonNodeName);

        for (int i = 0; i < serverList.GetLength(); i++)
        {
            AZStd::string oneHostAndOnePort(serverList.GetItem(i).AsString().c_str());
            AZStd::size_t portStartPosition = oneHostAndOnePort.find(':');
            if (portStartPosition == AZStd::string::npos)
            {
                // Returned data is not what we expected
                return false;
            }
            HostInfo hostInfo;
            hostInfo.address = oneHostAndOnePort.substr(0, portStartPosition);
            AZStd::string stringPort = oneHostAndOnePort.substr(portStartPosition + 1);
            hostInfo.port = ::atoi(stringPort.c_str());
            hostInfo.websocket = isWebsocket;
            hostInfo.ssl = chatPlay->GetVars()->IsPortSSL(hostInfo.port, isWebsocket);
            hostInfo.priority = chatPlay->GetVars()->GetPortPriority(hostInfo.port, isWebsocket);

            // Ignore priorities < 0
            if (hostInfo.priority >= 0)
            {
                hostInfoList.push_back(hostInfo);
            }
        }

        return true;
    }

}
