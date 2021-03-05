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
#ifndef CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAYCVARS_H
#define CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAYCVARS_H
#pragma once

// This class handles registering and unregistering the ChatPlay specific CVars.
// It also provides simple access to the values via typed accessors.
//
// The lifetime of the class is managed via shared_ptr; the vars are destroyed
// when there are no longer any references to ChatPlayVars.
//
// Note: ChatPlay holds a reference to ChatPlayVars as long as it is running.
//

namespace ChatPlay
{

    struct HostAndPort
    {
        HostAndPort(AZStd::string host, int port)
            : m_host(host)
            , m_port(port)
        {
        }

        AZStd::string m_host;
        int m_port;
    };

    struct HostInfo
    {
        AZStd::string address;
        int port              = 0;
        int priority          = -1;
        bool ssl              = false;
        bool websocket        = false;
        bool connectionFailed = false;

        bool IsValid()
        {
            return port != 0 && priority >= 0;
        }

        bool operator==(const HostInfo& hostInfo)
        {
            return this->port == hostInfo.port
                && this->priority == hostInfo.priority
                && this->ssl == hostInfo.ssl
                && this->websocket == hostInfo.websocket;
        }

        bool operator!=(const HostInfo& hostInfo)
        {
            return !(this->operator==(hostInfo));
        }
    };

    typedef AZStd::vector<HostAndPort> HostAndPortList;
    typedef AZStd::vector<HostInfo> HostInfoList;

    class ChatPlayCVars
    {
    public:
        static AZStd::shared_ptr<ChatPlayCVars> GetInstance();
        virtual ~ChatPlayCVars() = 0;

        HostAndPortList GetGroupHostsAndPorts();
        int GetPortPriority(int port, bool isWebsocket);
        bool IsPortSSL(int port, bool isWebsocket);

        static void ResetHostInfoFlags(HostInfoList& hostInfoList);

        bool IsEnabled() { return m_enabled != 0; }
        // TODO: These shouldn't be config vars (acquire them in-game)
        const char* GetUser() { return m_user; }
        const char* GetPassword() { return m_password; }
        const char* GetAPIServerAddress() { return m_apiServerAddress; }
        const char* GetClientID() { return m_clientID; }
        const char* GetIRCPortList() { return m_ircPortList; }
        const char* GetWebsocketPortList() { return m_websocketPortList; }

        virtual void RegisterCVars() = 0;
        virtual void UnregisterCVars() = 0;

    protected:

        int m_enabled;
        const char* m_user;
        const char* m_password;
        const char* m_apiServerAddress;
        const char* m_clientID;
        const char* m_ircPortList;
        const char* m_ircSSLPortList;
        const char* m_websocketPortList;
        const char* m_websocketSSLPortList;

        // Not for public use
        ChatPlayCVars();

    private:
        HostAndPortList ParseHostsAndPorts(AZStd::string HostAndPortToParse);

        HostAndPort ExtractPortHostPair(const AZStd::string& HostAndPortToParse, AZStd::size_t nextSplicePosition);
    };

    inline ChatPlayCVars::~ChatPlayCVars() {}
}

#endif // CRYINCLUDE_CRYACTION_CHATPLAY_CHATPLAYCVARS_H
