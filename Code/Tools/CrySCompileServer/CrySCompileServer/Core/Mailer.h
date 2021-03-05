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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSCOMPILESERVER_CORE_MAILER_H
#define CRYINCLUDE_CRYSCOMPILESERVER_CORE_MAILER_H
#pragma once

#include "Common.h"
#include "Server/CrySimpleSock.hpp"
#include <string>
#include <set>
#include <list>
#include <memory>
#include <AzCore/std/parallel/atomic.h>

class CSMTPMailer
{
public:
    typedef std::string tstr;
    typedef std::set<tstr> tstrcol;
    typedef std::pair<std::string, std::string> tattachment;
    typedef std::list<tattachment> tattachlist;

    static const int DEFAULT_PORT = 25;
    static AZStd::atomic_long ms_OpenSockets;

public:
    CSMTPMailer(const tstr& username, const tstr& password, const tstr& server, int port = DEFAULT_PORT);
    ~CSMTPMailer();

    bool Send(const tstr& from, const tstrcol& to, const tstrcol& cc, const tstrcol& bcc, const tstr& subject, const tstr& body, const tattachlist& attachments);
    const char* GetResponse() const;

    static long             GetOpenSockets() { return ms_OpenSockets; }

private:
    void ReceiveLine(SOCKET connection);
    void SendLine(SOCKET connection, const char* format, ...) const;
    void SendRaw(SOCKET connection, const char* data, size_t dataLen) const;
    void SendFile(SOCKET connection, const tattachment& file, const char* boundary) const;

    SOCKET Open(const char* host, unsigned short port,  sockaddr_in& serverAddress);

    void AddReceivers(SOCKET connection, const tstrcol& receivers);
    void AssignReceivers(SOCKET connection, const char* receiverTag, const tstrcol& receivers);
    void SendAttachments(SOCKET connection, const tattachlist& attachments, const char* boundary);

    bool IsEmpty(const tstrcol& col) const;

private:

    std::unique_ptr<CCrySimpleSock> m_socket;
    tstr m_server;
    tstr m_username;
    tstr m_password;
    int m_port;

    bool m_winSockAvail;
    tstr m_response;
};

#endif // CRYINCLUDE_CRYSCOMPILESERVER_CORE_MAILER_H
