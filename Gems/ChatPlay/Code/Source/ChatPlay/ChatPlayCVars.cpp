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

#include <string>
#include <sstream>
#include <cstdlib>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#include "ChatPlayCVars.h"

#define CHATPLAY_API_CLIENT_ID ""
#define CHATPLAY_API_SERVER_ADDRESS "tmi.twitch.tv"
#define CHATPLAY_IRC_PORTS "1:6667;3:80"
#define CHATPLAY_IRC_SSL_PORTS "-1:6697;-1:443"
#define CHATPLAY_WEBSOCKET_PORTS "2:80"
#define CHATPLAY_WEBSOCKET_SSL_PORTS "-1:443"

#define CHATPLAY_DEFAULT_USER "justinfan12345"
#define CHATPLAY_DEFAULT_PASSWORD "blah"

namespace ChatPlay
{

    /******************************************************************************/
    // Actual implementation of ChatPlayVars is private to this translation unit 

    namespace
    {
        class ChatPlayCVarsImpl
            : public ChatPlayCVars
        {
        public:
            ChatPlayCVarsImpl();
            ~ChatPlayCVarsImpl() override;

            void RegisterCVars() override;
            void UnregisterCVars() override;

        protected:
            // Stores a reference to each registered CVar for de-registration later
            std::list<ICVar*> m_vars;
        };
    }

    /******************************************************************************/
    // Implementation of ChatPlayCVars methods

    AZStd::shared_ptr<ChatPlayCVars> ChatPlayCVars::GetInstance()
    {
        // Singleton is stored here as static local variable; this is the only
        // global/static reference held by the system.
        static AZStd::weak_ptr<ChatPlayCVarsImpl> instance;

        // Attempt to lock the current instance or, if that fails, create a new one
        AZStd::shared_ptr<ChatPlayCVarsImpl> ptr = instance.lock();
        if (!ptr)
        {
            ptr = AZStd::make_shared<ChatPlayCVarsImpl>();
            instance = ptr;
        }

        return ptr;
    }


    HostAndPort ChatPlayCVars::ExtractPortHostPair(const AZStd::string& hostAndPortToParse, AZStd::size_t nextSplicePosition)
    {
        AZStd::string oneHostAndOnePort = hostAndPortToParse.substr(0, nextSplicePosition);
        AZStd::size_t portStartPosition = oneHostAndOnePort.find(':');
        AZStd::string host = oneHostAndOnePort.substr(0, portStartPosition);
        AZStd::string stringPort = oneHostAndOnePort.substr(portStartPosition + 1);
        int port = ::atoi(stringPort.c_str());
        HostAndPort hostAndPort(host, port);
        return hostAndPort;
    }

    HostAndPortList ChatPlayCVars::ParseHostsAndPorts(AZStd::string hostAndPortToParse)
    {
        HostAndPortList retVal;
        while (true)
        {
            AZStd::size_t nextSplicePosition = hostAndPortToParse.find(';');
            if (nextSplicePosition == AZStd::string::npos)
            {
                break;
            }
            retVal.push_back(ExtractPortHostPair(hostAndPortToParse, nextSplicePosition));
            hostAndPortToParse = hostAndPortToParse.substr(nextSplicePosition + 1);
        }
        return retVal;
    }

    int ChatPlayCVars::GetPortPriority(int port, bool isWebsocket)
    {
        AZStd::string ports = AZStd::string::format(
            "%s;%s",
            isWebsocket ? m_websocketPortList : m_ircPortList,
            isWebsocket ? m_websocketSSLPortList : m_ircSSLPortList
            );

        AZStd::size_t startPos = 0;
        AZStd::size_t splitPos;

        do
        {
            splitPos = ports.find(';', startPos);

            AZStd::string portAndPriority(ports.c_str(), startPos, splitPos - startPos);
            AZStd::size_t split = portAndPriority.find(':');

            if (split != AZStd::string::npos)
            {
                int parsedPriority = ::atoi(portAndPriority.substr(0, split).c_str());
                int parsedPort = ::atoi(portAndPriority.substr(split + 1).c_str());

                if (parsedPort == port)
                {
                    return parsedPriority;
                }
            }

            startPos = splitPos + 1;

        } while (splitPos != AZStd::string::npos);

        return -1;
    }

    bool ChatPlayCVars::IsPortSSL(int port, bool isWebsocket)
    {
        AZStd::string ports(isWebsocket ? m_websocketSSLPortList : m_ircSSLPortList);

        AZStd::size_t startPos = 0;
        AZStd::size_t splitPos;

        do
        {
            splitPos = ports.find(';', startPos);

            AZStd::string portAndPriority(ports.c_str(), startPos, splitPos - startPos);
            AZStd::size_t split = portAndPriority.find(':');

            if (split == AZStd::string::npos)
            {
                int parsedPort = ::atoi(portAndPriority.substr(split + 1).c_str());

                if (parsedPort == port)
                {
                    return true;
                }
            }

            startPos = splitPos + 1;
        } while (splitPos != AZStd::string::npos);

        return false;
    }

    void ChatPlayCVars::ResetHostInfoFlags(HostInfoList& hostInfoList)
    {
        for (HostInfo& hostInfo : hostInfoList)
        {
            hostInfo.connectionFailed = false;
        }
    }

    ChatPlayCVars::ChatPlayCVars()
        : m_enabled(1)
        , m_user(CHATPLAY_DEFAULT_USER)
        , m_password(CHATPLAY_DEFAULT_PASSWORD)
        , m_apiServerAddress(CHATPLAY_API_SERVER_ADDRESS)
        , m_clientID(CHATPLAY_API_CLIENT_ID)
        , m_ircPortList(CHATPLAY_IRC_PORTS)
        , m_ircSSLPortList(CHATPLAY_IRC_SSL_PORTS)
        , m_websocketPortList(CHATPLAY_WEBSOCKET_PORTS)
        , m_websocketSSLPortList(CHATPLAY_WEBSOCKET_SSL_PORTS)
    {
    }

    /******************************************************************************/
    // Implementation of ChatPlayCVarsImpl methods

    ChatPlayCVarsImpl::ChatPlayCVarsImpl()
    {
    }

    ChatPlayCVarsImpl::~ChatPlayCVarsImpl()
    {
    }

    void ChatPlayCVarsImpl::RegisterCVars()
    {
        ICVar* ptr;

        ptr = REGISTER_CVAR2("chatPlay_Enabled", &m_enabled, 1, VF_NULL, "Set to 0 to disable ChatPlay.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_UserName", &m_user, CHATPLAY_DEFAULT_USER, VF_NULL, "The username for ChatPlay to log into the IRC with.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_Password", &m_password, CHATPLAY_DEFAULT_PASSWORD, VF_NULL, "The password for ChatPlay to log into the IRC with.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_ServerListEndpoint", &m_apiServerAddress, CHATPLAY_API_SERVER_ADDRESS, VF_NULL, "The API's server address used for retrieving chat server IPs and ports.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_ClientID", &m_clientID, CHATPLAY_API_CLIENT_ID, VF_NULL, "The Client-ID for making ChatPlay API requests.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_IRCPorts", &m_ircPortList, CHATPLAY_IRC_PORTS, VF_NULL, "The list of ports and their priorities used for connecting to Twitch IRC.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_IRCSSLPorts", &m_ircSSLPortList, CHATPLAY_IRC_SSL_PORTS, VF_NULL, "The list of ports and their priorities used for connecting to Twitch IRC over SSL.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_WebsocketPorts", &m_websocketPortList, CHATPLAY_WEBSOCKET_PORTS, VF_NULL, "The list of ports and their priorities used for connecting to Twitch IRC over websockets.");
        m_vars.push_back(ptr);

        ptr = REGISTER_CVAR2("chatPlay_WebsocketSSLPorts", &m_websocketSSLPortList, CHATPLAY_WEBSOCKET_SSL_PORTS, VF_NULL, "The list of ports and their priorities used for connecting to Twitch IRC over secure websockets.");
        m_vars.push_back(ptr);
    }

    void ChatPlayCVarsImpl::UnregisterCVars()
    {
        for (ICVar* v : m_vars)
        {
            string name = v->GetName();
            UNREGISTER_CVAR(name);
        }

        m_vars.clear();
    }

}