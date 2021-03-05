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

#include "Mailer.h"
#include "WindowsAPIImplementation.h"

#include <AzCore/PlatformDef.h>
#include <assert.h>

#if defined(AZ_PLATFORM_MAC)
#include <netdb.h>
#include <unistd.h>
#elif defined(AZ_PLATFORM_WINDOWS)
#include <ws2tcpip.h>
#endif

#include <AzCore/base.h>
#include <AzCore/IO/SystemFile.h>

#pragma comment(lib,"ws2_32.lib")


namespace // helpers
{
    static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


    void Base64EncodeBlock(const unsigned char* in, unsigned char* out)
    {
        out[0] = cb64[in[0] >> 2];
        out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
        out[2] = cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)];
        out[3] = cb64[in[2] & 0x3f];
    }


    void Base64EncodeBlock(const unsigned char* in, unsigned char* out, int len)
    {
        out[0] = cb64[in[0] >> 2];
        out[1] = cb64[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
        out[2] = (unsigned char) (len > 1 ? cb64[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=');
        out[3] = (unsigned char) (len > 2 ? cb64[in[2] & 0x3f] : '=');
    }


    void Base64Encode(const unsigned char* pSrc, const size_t srcLen, unsigned char* pDst, [[maybe_unused]] const size_t dstLen)
    {
        assert(dstLen >= 4 * ((srcLen + 2) / 3));

        size_t len = srcLen;
        for (; len > 2; len -= 3, pSrc += 3, pDst += 4)
        {
            Base64EncodeBlock(pSrc, pDst);
        }

        if (len > 0)
        {
            unsigned char in[3];
            in[0] = pSrc[0];
            in[1] = len > 1 ? pSrc[1] : 0;
            in[2] = 0;
            Base64EncodeBlock(in, pDst, (int) len);
        }
    }


    std::string Base64EncodeString(const std::string& in)
    {
        const size_t srcLen = in.size();
        const size_t dstLen = 4 * ((srcLen + 2) / 3);
        std::string out(dstLen, 0);

        Base64Encode((const unsigned char*) in.c_str(), srcLen, (unsigned char*) out.c_str(), dstLen);

        return out;
    }


    const char* ExtractFileName(const char* filepath)
    {
        for (const char* p = filepath + strlen(filepath) - 1; p >= filepath; --p)
        {
            if (*p == '\\' || *p == '/')
            {
                return p + 1;
            }
        }
        return filepath;
    }
}

AZStd::atomic_long CSMTPMailer::ms_OpenSockets = {0};

CSMTPMailer::CSMTPMailer(const tstr& username, const tstr& password, const tstr& server, int port)
    : m_server(server)
    , m_username(username)
    , m_password(password)
    , m_port(port)
    , m_winSockAvail(false)
    , m_response()
{
#if defined(AZ_PLATFORM_WINDOWS)
    WSADATA wd;
    m_winSockAvail = WSAStartup(MAKEWORD(1, 1), &wd) == 0;
    if (!m_winSockAvail)
    {
        m_response += "Error: Unable to initialize WinSock 1.1\n";
    }
#endif
}


CSMTPMailer::~CSMTPMailer()
{
#if defined(AZ_PLATFORM_WINDOWS)
    if (m_winSockAvail)
    {
        WSACleanup();
    }
#endif
}


void CSMTPMailer::ReceiveLine(SOCKET connection)
{
    char buf[1025];
    int ret = recv(connection, buf, sizeof(buf) - 1, 0);
    if (ret == SOCKET_ERROR)
    {
        ret = azsnprintf(buf, sizeof(buf), "Error: WinSock error %d during recv()\n", WSAGetLastError());
        if (ret == sizeof(buf) || ret < 0)
        {
            buf[sizeof(buf) - 1] = '\0';
        }
    }
    else
    {
        buf[ret] = 0;
    }
    m_response += buf;
}


void CSMTPMailer::SendLine(SOCKET connection, const char* format, ...) const
{
    char buf[2049];
    va_list args;
    va_start(args, format);
    int len = azvsnprintf(buf, sizeof(buf), format, args);
    if (len == sizeof(buf) || len < 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        len = sizeof(buf) - 1;
    }
    va_end(args);
    send(connection, buf, len, 0);
}


void CSMTPMailer::SendRaw(SOCKET connection, const char* data, size_t dataLen) const
{
    send(connection, data, (int) dataLen, 0);
}


void CSMTPMailer::SendFile(SOCKET connection, const tattachment& filepath, const char* boundary) const
{
    AZ::IO::SystemFile inputFile;
    const bool wasSuccessful = inputFile.Open(filepath.second.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
    if (wasSuccessful)
    {
        SendLine(connection, "--%s\r\n", boundary);
        SendLine(connection, "Content-Type: application/octet-stream\r\n");
        SendLine(connection, "Content-Transfer-Encoding: base64\r\n");
        SendLine(connection, "Content-Disposition: attachment; filename=\"%s\"\r\n", filepath.first.c_str());
        SendLine(connection, "\r\n");

        AZ::IO::SystemFile::SizeType fileSize = inputFile.Length();
        while (fileSize)
        {
            const int DEF_BLOCK_SIZE = 128; // 72
            char in[3 * DEF_BLOCK_SIZE];
            size_t blockSize = fileSize > sizeof(in) ? sizeof(in) : fileSize;
            inputFile.Read(blockSize, in);

            char out[4 * DEF_BLOCK_SIZE];
            Base64Encode((unsigned char*) in, blockSize, (unsigned char*) out, sizeof(out));
            SendRaw(connection, out, 4 * ((blockSize + 2) / 3));

            SendLine(connection, "\r\n"); // seems to get sent faster if you split up the data lines

            fileSize -= blockSize;
        }
    }
}


SOCKET CSMTPMailer::Open(const char* host, unsigned short port, sockaddr_in& serverAddress)
{
    SOCKET connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connection == INVALID_SOCKET)
    {
        m_response += "Error: Failed to create socket\n";
        return 0;
    }

    struct addrinfo* addressInfo{};
    char portBuffer[16];
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    azsnprintf(portBuffer, AZStd::size(portBuffer), "%hu", port);
    int addrInfoReturnCode = getaddrinfo(host, portBuffer, &hints, &addressInfo);
    if(addrInfoReturnCode != 0)
    {
        char buf[1025];
        int ret = azsnprintf(buf, sizeof(buf), "Error: Host %s not found\n", host);
        if (ret == sizeof(buf) || ret < 0)
        {
            buf[sizeof(buf) - 1] = '\0';
        }
        m_response += buf;
        closesocket(connection);
        return 0;
    }

    if (addressInfo)
    {
        ++ms_OpenSockets;

        serverAddress = *reinterpret_cast<sockaddr_in*>(addressInfo->ai_addr);
    }

    return connection;
}


void CSMTPMailer::AddReceivers(SOCKET connection, const tstrcol& receivers)
{
    for (tstrcol::const_iterator it = receivers.begin(), itEnd = receivers.end(); it != itEnd; ++it)
    {
        if (!(*it).empty())
        {
            SendLine(connection, "rcpt to: %s\r\n", (*it).c_str());
            ReceiveLine(connection);
        }
    }
}


void CSMTPMailer::AssignReceivers(SOCKET connection, const char* receiverTag, const tstrcol& receivers)
{
    tstrcol::const_iterator it = receivers.begin();
    tstrcol::const_iterator itEnd = receivers.end();

    while (it != itEnd && (*it).empty())
    {
        ++it;
    }

    if (it != itEnd)
    {
        tstr out(receiverTag);
        out += *it;
        ++it;
        for (; it != itEnd; ++it)
        {
            if (!(*it).empty())
            {
                out += "; ";
                out += *it;
            }
        }
        out += "\r\n";
        SendLine(connection, out.c_str());
    }
}


void CSMTPMailer::SendAttachments(SOCKET connection, const tattachlist& attachments, const char* boundary)
{
    for (tattachlist::const_iterator it = attachments.begin(), itEnd = attachments.end(); it != itEnd; ++it)
    {
        if (!(*it).first.empty() && !(*it).second.empty())
        {
            SendFile(connection, *it, boundary);
        }
    }
}


bool CSMTPMailer::IsEmpty(const tstrcol& col) const
{
    if (!col.empty())
    {
        for (tstrcol::const_iterator it = col.begin(), itEnd = col.end(); it != itEnd; ++it)
        {
            if (!(*it).empty())
            {
                return false;
            }
        }
    }

    return true;
}


bool CSMTPMailer::Send(const tstr& from, const tstrcol& to, const tstrcol& cc, const tstrcol& bcc, const tstr& subject, const tstr& body, const tattachlist& attachments)
{
    if (!m_winSockAvail)
    {
        return false;
    }

    if (from.empty() || IsEmpty(to))
    {
        return false;
    }

    sockaddr_in serverAddress;
    SOCKET connection = Open(m_server.c_str(), m_port, serverAddress); // SMTP telnet (usually port 25)
    if (connection == INVALID_SOCKET)
    {
        return false;
    }

    if (connect(connection, (sockaddr*) &serverAddress, sizeof(serverAddress)) != SOCKET_ERROR)
    {
        ReceiveLine(connection);

        SendLine(connection, "helo localhost\r\n");
        ReceiveLine(connection);

        if (!m_username.empty() && !m_password.empty())
        {
            SendLine(connection, "auth login\r\n"); // most servers should implement this (todo: otherwise fall back to PLAIN or CRAM-MD5 (requiring EHLO))
            ReceiveLine(connection);
            SendLine(connection, "%s\r\n", Base64EncodeString(m_username).c_str());
            ReceiveLine(connection);
            SendLine(connection, "%s\r\n", Base64EncodeString(m_password).c_str());
            ReceiveLine(connection);
        }

        SendLine(connection, "mail from: %s\r\n", from.c_str());
        ReceiveLine(connection);

        AddReceivers(connection, to);
        AddReceivers(connection, cc);
        AddReceivers(connection, bcc);

        SendLine(connection, "data\r\n");
        ReceiveLine(connection);

        SendLine(connection, "From: %s\r\n", from.c_str());
        AssignReceivers(connection, "To: ", to);
        AssignReceivers(connection, "Cc: ", cc);
        AssignReceivers(connection, "Bcc: ", bcc);

        SendLine(connection, "Subject: %s\r\n", subject.c_str());

        static const char boundary[] = "------a95ed0b485e4a9b0fd4ff93f50ad06ca"; // beware, boundary should not clash with text content of message body!

        SendLine(connection, "MIME-Version: 1.0\r\n");
        SendLine(connection, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n", boundary);
        SendLine(connection, "\r\n");
        SendLine(connection, "This is a multi-part message in MIME format.\r\n");

        SendLine(connection, "--%s\r\n", boundary);
        SendLine(connection, "Content-Type: text/plain; charset=iso-8859-1; format=flowed\r\n"); // the used charset should support the commonly used special characters of western languages
        SendLine(connection, "Content-Transfer-Encoding: 7bit\r\n");
        SendLine(connection, "\r\n");
        SendRaw(connection, body.c_str(), body.size());
        SendLine(connection, "\r\n");

        SendAttachments(connection, attachments, boundary);

        SendLine(connection, "--%s--\r\n", boundary);

        SendLine(connection, "\r\n.\r\n");
        ReceiveLine(connection);

        SendLine(connection, "quit\r\n");
        ReceiveLine(connection);
    }
    else
    {
        char buf[1025];
        int ret = azsnprintf(buf, sizeof(buf), "Error: Failed to connect to %s:%d\n", m_server.c_str(), m_port);
        if (ret == sizeof(buf) || ret < 0)
        {
            buf[sizeof(buf) - 1] = '\0';
        }
        m_response += buf;
        return false;
    }

    closesocket(connection);
    --ms_OpenSockets;

    return true;
}


const char* CSMTPMailer::GetResponse() const
{
    return m_response.c_str();
}
