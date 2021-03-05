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

#include "CrySystem_precompiled.h"
#include "NotificationNetwork.h"
#include <ISystem.h>

#include <AzCore/Socket/AzSocket.h>

#undef LockDebug
//#define LockDebug(str1,str2)  {string strMessage;strMessage.Format(str1,str2);if (m_clients.size())   OutputDebugString(strMessage.c_str());}
#define LockDebug(str1, str2)

//

extern bool RCON_IsRemoteAllowedToConnect(const AZ::AzSock::AzSocketAddress& connectee);

using namespace NotificationNetwork;

//

#include <CryPath.h>
class CQueryNotification
    : public INotificationNetworkListener
{
    // INotificationNetworkListener
public:
    virtual void OnNotificationNetworkReceive([[maybe_unused]] const void* pBuffer, [[maybe_unused]] size_t length)
    {
        INotificationNetwork* pNotificationNetwork =
            gEnv->pSystem->GetINotificationNetwork();
        if (!pNotificationNetwork)
        {
            return;
        }

        const char* path = nullptr; // Don't call GetGameFolder here, it returns a full absolute path and we just really want the game name

        if (ICVar* pVar = gEnv->pConsole->GetCVar("sys_game_folder"))
        {
            path = pVar->GetString();
        }
        if (!path)
        {
            return;
        }

        pNotificationNetwork->Send("SystemInfo", path, ::strlen(path) + 1);
    }
} g_queryNotification;

AZSOCKET CConnectionBase::CreateSocket()
{
    AZSOCKET sock = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(sock))
    {
        CryLog("CNotificationNetworkClient::Create: Failed to create socket.");
        return AZ_SOCKET_INVALID;
    }

    if (AZ::AzSock::SetSocketOption(sock, AZ::AzSock::AzSocketOption::REUSEADDR, true))
    {
        AZ::AzSock::CloseSocket(sock);
        CryLog("CNotificationNetworkClient::Create: Failed to set SO_REUSEADDR option.");
        return AZ_SOCKET_INVALID;
    }

#if defined (WIN32) || defined(WIN64) //MS Platforms
    if (AZ::AzSock::SetSocketBlockingMode(sock, false))
    {
        AZ::AzSock::CloseSocket(sock);
        CryLog("CNotificationNetworkClient::Connect: Failed to set socket to asynchronous operation.");
        return AZ_SOCKET_INVALID;
    }

#endif

    // TCP_NODELAY required for win32 because of high latency connection otherwise
#if defined(WIN32)
    if (AZ::AzSock::EnableTCPNoDelay(sock, true))
    {
        AZ::AzSock::CloseSocket(sock);
        CryLog("CNotificationNetworkClient::Create: Failed to set TCP_NODELAY option.");
        return AZ_SOCKET_INVALID;
    }
#endif

    return sock;
}

bool CConnectionBase::Connect(const char* address, uint16 port)
{
    AZ::AzSock::AzSocketAddress socketAddress;
    socketAddress.SetAddress(address, port);

    int result = AZ::AzSock::Connect(m_socket, socketAddress);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        AZ::AzSock::AzSockError err = AZ::AzSock::AzSockError(result);
        if (err == AZ::AzSock::AzSockError::eASE_EWOULDBLOCK_CONN)
        {
            return true;
        }

        if (err == AZ::AzSock::AzSockError::eASE_EISCONN)
        {
            if (!m_boIsConnected)
            {
                m_boIsConnected = true;
                m_boIsFailedToConnect = false;
                OnConnect(true);
            }
            return true;
        }

        if (err == AZ::AzSock::AzSockError::eASE_EALREADY)
        {
            // It will happen, in case of DNS problems, or if the console is not
            // reachable or turned off.
            //CryLog("CNotificationNetworkClient::Connect: Failed to connect. Reason: already conencted.");
            return true;
        }

        AZ::AzSock::CloseSocket(m_socket);
        m_socket = AZ_SOCKET_INVALID;
        CryLog("CNotificationNetworkClient::Connect: Failed to connect. Reason: %d ", result);
        return false;
    }

    return true;
}

/*

  CChannel

*/

bool CChannel::IsNameValid(const char* name)
{
    if (!name)
    {
        return false;
    }
    if (!*name)
    {
        return false;
    }

    if (::strlen(name) > NN_CHANNEL_NAME_LENGTH_MAX)
    {
        return false;
    }

    return true;
}

//

CChannel::CChannel()
{
}

CChannel::CChannel(const char* name)
{
    if (!name)
    {
        return;
    }

    if (!*name)
    {
        return;
    }

    size_t length = MIN(::strlen(name), NN_CHANNEL_NAME_LENGTH_MAX);
    ::memset(m_name, 0, NN_CHANNEL_NAME_LENGTH_MAX);
    ::memcpy(m_name, name, length);
}

CChannel::~CChannel()
{
}

//

void CChannel::WriteToPacketHeader(void* pPacket) const
{
    ::memcpy((uint8*)pPacket + NN_PACKET_HEADER_OFFSET_CHANNEL,
        m_name, NN_CHANNEL_NAME_LENGTH_MAX);
}

void CChannel::ReadFromPacketHeader(void* pPacket)
{
    ::memcpy(m_name, (uint8*)pPacket + NN_PACKET_HEADER_OFFSET_CHANNEL,
        NN_CHANNEL_NAME_LENGTH_MAX);
}

//

bool CChannel::operator ==(const CChannel& channel) const
{
    return ::strncmp(m_name, channel.m_name, NN_CHANNEL_NAME_LENGTH_MAX) == 0;
}

bool CChannel::operator !=(const CChannel& channel) const
{
    return ::strncmp(m_name, channel.m_name, NN_CHANNEL_NAME_LENGTH_MAX) != 0;
}

/*

  CListeners

*/

CListeners::CListeners()
{
    m_pNotificationWrite = &m_notifications[0];
    m_pNotificationRead = &m_notifications[1];
}

CListeners::~CListeners()
{
    while (!m_pNotificationRead->empty())
    {
        SBuffer buffer = m_pNotificationRead->front();
        m_pNotificationRead->pop();
        delete[] buffer.pData;
    }

    while (!m_pNotificationWrite->empty())
    {
        SBuffer buffer = m_pNotificationWrite->front();
        m_pNotificationWrite->pop();
        delete[] buffer.pData;
    }
}

//

size_t CListeners::Count(const CChannel& channel)
{
    size_t count = 0;
    for (size_t i = 0; i < m_listeners.size(); ++i)
    {
        if (m_listeners[i].second != channel)
        {
            continue;
        }

        ++count;
    }

    return count;
}

CChannel* CListeners::Channel(INotificationNetworkListener* pListener)
{
    for (size_t i = 0; i < m_listeners.size(); ++i)
    {
        if (m_listeners[i].first != pListener)
        {
            continue;
        }

        return &m_listeners[i].second;
    }

    return nullptr;
}

bool CListeners::Bind(const CChannel& channel, INotificationNetworkListener* pListener)
{
    for (size_t i = 0; i < m_listeners.size(); ++i)
    {
        if (m_listeners[i].first == pListener)
        {
            m_listeners[i].second = channel;
            return true;
        }
    }

    m_listeners.push_back(std::pair<INotificationNetworkListener*, CChannel>());
    m_listeners.back().first = pListener;
    m_listeners.back().second = channel;
    return true;
}

bool CListeners::Remove(INotificationNetworkListener* pListener)
{
    for (size_t i = 0; i < m_listeners.size(); ++i)
    {
        if (m_listeners[i].first != pListener)
        {
            continue;
        }

        m_listeners[i] = m_listeners.back();
        m_listeners.pop_back();
        return true;
    }

    return false;
}

void CListeners::NotificationPush(const SBuffer& buffer)
{
    // TODO: Use auto lock.
    m_notificationCriticalSection.Lock();
    m_pNotificationWrite->push(buffer);
    m_notificationCriticalSection.Unlock();
}

void CListeners::NotificationsProcess()
{
    m_notificationCriticalSection.Lock();
    std::swap(m_pNotificationWrite, m_pNotificationRead);
    m_notificationCriticalSection.Unlock();

    while (!m_pNotificationRead->empty())
    {
        SBuffer buffer = m_pNotificationRead->front();
        m_pNotificationRead->pop();

        for (size_t i = 0; i < m_listeners.size(); ++i)
        {
            if (m_listeners[i].second != buffer.channel)
            {
                continue;
            }

            m_listeners[i].first->OnNotificationNetworkReceive(
                buffer.pData, buffer.length);
        }

        delete[] buffer.pData;
    }
}

/*

  CConnectionBase

*/

CConnectionBase::CConnectionBase(CNotificationNetwork* pNotificationNetwork)
{
    m_pNotificationNetwork = pNotificationNetwork;

    m_port = 0;

    m_socket = AZ_SOCKET_INVALID;

    m_buffer.pData = nullptr;
    m_buffer.length = 0;
    m_dataLeft = 0;

    m_boIsConnected = false;
    m_boIsFailedToConnect = false;
}

CConnectionBase::~CConnectionBase()
{
    if (m_buffer.pData)
    {
        delete[] m_buffer.pData;
    }

    if (m_socket != AZ_SOCKET_INVALID)
    {
        CloseSocket_Internal();
    }
}

//

void CConnectionBase::SetAddress(const char* address, uint16 port)
{
    size_t length = MIN(::strlen(address), 15);
    ::memset(m_address, 0, sizeof(m_address));
    ::memcpy(m_address, address, length);
    m_port = port;
}

bool CConnectionBase::Validate()
{
    if (m_socket != AZ_SOCKET_INVALID)
    {
        if (!m_port)
        {
            AZ::AzSock::AzSocketAddress socketAddress;
            int result = AZ::AzSock::GetSockName(m_socket, socketAddress);
            if (AZ::AzSock::SocketErrorOccured(result))
            {
                return false;
            }
        }

        return Select_Internal();
    }

    if (!m_port) // If port is not set we don't want to try to reconnect.
    {
        return false;
    }

    m_socket = CreateSocket();
    // If the create sockect will fail, it is likely that we will never be able to connect,
    // we might want to signal that.

    Connect(m_address, m_port);

    return false;
}

bool CConnectionBase::Send(const void* pBuffer, size_t length)
{
    if (!Validate())
    {
        return false;
    }

    size_t sent = 0;
    while (sent < length)
    {
        int r = AZ::AzSock::Send(m_socket, (const char*)pBuffer + sent, length - sent, 0);
        if (AZ::AzSock::SocketErrorOccured(r))
        {
            AZ::AzSock::AzSockError nCurrentError = AZ::AzSock::AzSockError(r);
            if  (nCurrentError == AZ::AzSock::AzSockError::eASE_ENOTCONN)
            {
                r = 0;
                break;
            }
            else if (nCurrentError == AZ::AzSock::AzSockError::eASE_EWOULDBLOCK)
            {
                r = 0;
            }
            else
            {
                CryLog("CNotificationNetworkClient::Send: Failed to send package. Reason: %s", AZ::AzSock::GetStringForError(r));
                CloseSocket_Internal();
                return false;
            }
        }

        sent += r;
    }

    return true;
}

bool CConnectionBase::SendMessage(EMessage eMessage, const CChannel& channel, uint32 data)
{
    char header[NN_PACKET_HEADER_LENGTH];
    ::memset(header, 0, NN_PACKET_HEADER_LENGTH);
    *(uint32*)&header[NN_PACKET_HEADER_OFFSET_MESSAGE] = AZ::AzSock::HostToNetLong(eMessage);
    *(uint32*)&header[NN_PACKET_HEADER_OFFSET_DATA_LENGTH] = AZ::AzSock::HostToNetLong(data);
    channel.WriteToPacketHeader(header);

    if (!Send(header, NN_PACKET_HEADER_LENGTH))
    {
        return false;
    }

    return true;
}

bool CConnectionBase::Select_Internal()
{
    if (m_socket == AZ_SOCKET_INVALID)
    {
        return false;
    }

    AZFD_SET stExceptions;
    AZFD_SET stWriteSockets;

    FD_ZERO(&stExceptions);
    FD_SET(m_socket, &stExceptions);

    FD_ZERO(&stWriteSockets);
    FD_SET(m_socket, &stWriteSockets);

    AZTIMEVAL timeOut = { 0, 0 };

    int r = AZ::AzSock::Select(m_socket, nullptr, &stWriteSockets, &stExceptions, &timeOut);
    if (AZ::AzSock::SocketErrorOccured(r))
    {
        CryLog("CNotificationNetworkClient:: Failed to select socket. Reason: %s", AZ::AzSock::GetStringForError(r));
        CloseSocket_Internal();
        m_boIsFailedToConnect = true;
        return false;
    }
    else if (!r)
    {
        return m_boIsConnected;
    }

    if (FD_ISSET(m_socket, &stExceptions))
    {
        CloseSocket_Internal();
        m_boIsFailedToConnect = true;
        OnConnect(m_boIsConnected); // Handles failed attempt to connect.
        return false;
    }
    else if (FD_ISSET(m_socket, &stWriteSockets)) // In Windows a socket can be in both lists.
    {
        if (!m_boIsConnected)
        {
            m_boIsConnected = true;
            m_boIsFailedToConnect = false;
            OnConnect(m_boIsConnected); // Handles successful attempt to connect.
        }
        return true;
    }

    return false;
}

void CConnectionBase::CloseSocket_Internal()
{
    AZ::AzSock::CloseSocket(m_socket);
    m_socket = AZ_SOCKET_INVALID;
    if (m_boIsConnected)
    {
        OnDisconnect();
    }
    m_boIsConnected = false;
}

bool CConnectionBase::SendNotification(const CChannel& channel, const void* pBuffer, size_t length)
{
    if (!SendMessage(eMessage_DataTransfer, channel, length))
    {
        return false;
    }
    if (!length)
    {
        return true;
    }

    if (!Send(pBuffer, length))
    {
        return false;
    }

    return true;
}

bool CConnectionBase::ReceiveMessage(CListeners& listeners)
{
    if (!Validate())
    {
        return false;
    }

    if (!m_dataLeft)
    {
        m_dataLeft = NN_PACKET_HEADER_LENGTH;
    }
    int r = AZ::AzSock::Recv(m_socket, (char*)&m_bufferHeader[NN_PACKET_HEADER_LENGTH - m_dataLeft], m_dataLeft, 0);
    if (!r)
    {
        // Connection terminated.
        m_dataLeft = 0;

        CloseSocket_Internal();
        return false;
    }
    if (AZ::AzSock::SocketErrorOccured(r))
    {
        m_dataLeft = 0;

        CryLog("CNotificationNetworkClient::ReceiveMessage: Failed to receive package. Reason: %s", AZ::AzSock::GetStringForError(r));
        CloseSocket_Internal();
        return false;
    }

    if (m_dataLeft -= r)
    {
        return true;
    }

    // The whole message was received, process it...

    EMessage eMessage = (EMessage)AZ::AzSock::NetToHostLong(
        *(uint32*)&m_bufferHeader[NN_PACKET_HEADER_OFFSET_MESSAGE]);
    const CChannel& channel = *(CChannel*)&m_bufferHeader[NN_PACKET_HEADER_OFFSET_CHANNEL];

    if (eMessage == eMessage_DataTransfer)
    {
        m_dataLeft = AZ::AzSock::NetToHostLong(*(uint32*)&m_bufferHeader[NN_PACKET_HEADER_OFFSET_DATA_LENGTH]);
        if (!m_dataLeft)
        {
            SBuffer buffer;
            buffer.channel = channel;
            buffer.pData = nullptr;
            buffer.length = 0;
            listeners.NotificationPush(buffer);
            return true;
        }

        m_buffer.pData = new uint8[m_buffer.length = m_dataLeft];
        if (!m_buffer.pData)
        {
            CryLog("CNotificationNetwork::CConnection::Receive: Failed to allocate buffer.\n");
            m_dataLeft = 0;

            CloseSocket_Internal();
            return false;
        }

        m_buffer.channel.ReadFromPacketHeader(m_bufferHeader);
        return +1;
    }

    if (!OnMessage(eMessage, channel))
    {
        CryLog("NotificationNetwork::CConnectionBase::ReceiveMessage: "
            "Unknown message received, terminating Connection...\n");
        m_dataLeft = 0;

        CloseSocket_Internal();
        return false;
    }

    return true;
}

bool CConnectionBase::ReceiveNotification(CListeners& listeners)
{
    int r = AZ::AzSock::Recv(m_socket, (char*)&m_buffer.pData[m_buffer.length - m_dataLeft], m_dataLeft, 0);
    if (!r)
    {
        CryLog("CNotificationNetworkClient::ReceiveNotification: Failed to receive package. Reason: Connection terminated.");
        // Connection terminated.
        m_dataLeft = 0;

        CloseSocket_Internal();
        return false;
    }

    if (AZ::AzSock::SocketErrorOccured(r))
    {
        m_dataLeft = 0;

        CryLog("CNotificationNetworkClient::ReceiveNotification: Failed to receive package. Reason: %s", AZ::AzSock::GetStringForError(r));
        CloseSocket_Internal();
        return false;
    }

    if (m_dataLeft -= r)
    {
        return true;
    }

    listeners.NotificationPush(m_buffer);
    m_buffer.pData = nullptr;
    m_buffer.length = 0;
    m_dataLeft = 0;
    return true;
}

bool CConnectionBase::Receive(CListeners& listeners)
{
    if (m_buffer.pData)
    {
        return ReceiveNotification(listeners);
    }

    return ReceiveMessage(listeners);
}

bool CConnectionBase::GetIsConnectedFlag()
{
    return Select_Internal() || m_boIsConnected;
}

bool CConnectionBase::GetIsFailedToConnectFlag() const
{
    return m_boIsFailedToConnect;
}

/*

  CClient

*/

CClient* CClient::Create(CNotificationNetwork* pNotificationNetwork, const char* address, uint16 port)
{
    CClient* pClient = new CClient(pNotificationNetwork);
    AZSOCKET sock = pClient->CreateSocket();
    // In the current implementation, this is REALLY UNLIKELY to happen.
    if (sock == AZ_SOCKET_INVALID)
    {
        delete pClient;
        return nullptr;
    }

    //
    pClient->SetSocket(sock);
    pClient->Connect(address, port);

    pClient->SetAddress(address, port);
    pClient->SetSocket(sock);
    return pClient;
}

CClient* CClient::Create(CNotificationNetwork* pNotificationNetwork)
{
    CClient* pClient = new CClient(pNotificationNetwork);
    return pClient;
}

//

CClient::CClient(CNotificationNetwork* pNotificationNetwork)
    : CConnectionBase(pNotificationNetwork)
{
}

CClient::~CClient()
{
    GetNotificationNetwork()->ReleaseClients(this);
}

//

void CClient::Update()
{
    m_listeners.NotificationsProcess();
}

// CConnectionBase

bool CClient::OnConnect(bool boConnected)
{
    if (boConnected)
    {
        for (size_t i = 0; i < m_listeners.Count(); ++i)
        {
            if (!SendMessage(eMessage_ChannelRegister, m_listeners.Channel(i), 0))
            {
                return false;
            }
        }
    }

    CryAutoLock<CryCriticalSection> lock(m_stConnectionCallbacksLock);
    for (size_t nCount = 0; nCount < m_cNotificationNetworkConnectionCallbacks.size(); ++nCount)
    {
        m_cNotificationNetworkConnectionCallbacks[nCount]->OnConnect(this, boConnected);
    }

    return boConnected;
}

bool CClient::OnDisconnect()
{
    CryAutoLock<CryCriticalSection> lock(m_stConnectionCallbacksLock);
    for (size_t nCount = 0; nCount < m_cNotificationNetworkConnectionCallbacks.size(); ++nCount)
    {
        m_cNotificationNetworkConnectionCallbacks[nCount]->OnDisconnected(this);
    }

    return true;
}

bool CClient::OnMessage([[maybe_unused]] EMessage eMessage, [[maybe_unused]] const CChannel& channel)
{
    return false;
}

// INotificationNetworkClient

bool CClient::Connect(const char* address, uint16 port)
{
    bool bReturnValue(false);

    if (m_socket == AZ_SOCKET_INVALID)
    {
        m_socket = CreateSocket();
    }

    bReturnValue = CConnectionBase::Connect(address, port);
    if (bReturnValue)
    {
        SetAddress(address, port);
    }

    return bReturnValue;
}

bool CClient::ListenerBind(const char* channelName, INotificationNetworkListener* pListener)
{
    if (!CChannel::IsNameValid(channelName))
    {
        return false;
    }

    if (!m_listeners.Bind(CChannel(channelName), pListener))
    {
        return false;
    }

    if (!SendMessage(eMessage_ChannelRegister, CChannel(channelName), 0))
    {
        return false;
    }

    return true;
}

bool CClient::ListenerRemove(INotificationNetworkListener* pListener)
{
    CChannel* pChannel = m_listeners.Channel(pListener);
    if (!pChannel)
    {
        return false;
    }

    if (!m_listeners.Remove(pListener))
    {
        return false;
    }

    if (!SendMessage(eMessage_ChannelUnregister, *pChannel, 0))
    {
        return false;
    }

    return true;
}

bool CClient::Send(const char* channelName, const void* pBuffer, size_t length)
{
    CRY_ASSERT(CChannel::IsNameValid(channelName));
    //  CRY_ASSERT_MESSAGE(channelLength <= NN_CHANNEL_NAME_LENGTH_MAX,
    //      "Channel name \"%s\" was passed to a Notification Network method, the name cannot be longer than %d chars.",
    //      channel, NN_CHANNEL_NAME_LENGTH_MAX);

    if (!CChannel::IsNameValid(channelName))
    {
        return false;
    }
    if (!SendNotification(CChannel(channelName), pBuffer, length))
    {
        return false;
    }

    return true;
}

bool CClient::RegisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback)
{
    CryAutoLock<CryCriticalSection> lock(m_stConnectionCallbacksLock);
    return stl::push_back_unique(m_cNotificationNetworkConnectionCallbacks, pConnectionCallback);
}

bool CClient::UnregisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback)
{
    CryAutoLock<CryCriticalSection> lock(m_stConnectionCallbacksLock);
    return stl::find_and_erase(m_cNotificationNetworkConnectionCallbacks, pConnectionCallback);
}

/*

  CNotificationNetwork::CConnection

*/

CNotificationNetwork::CConnection::CConnection(CNotificationNetwork* pNotificationNetwork, AZSOCKET sock)
    : CConnectionBase(pNotificationNetwork)
{
    SetSocket(sock);
    m_listeningChannels.reserve(8);
}

CNotificationNetwork::CConnection::~CConnection()
{
}

//

bool CNotificationNetwork::CConnection::IsListening(const CChannel& channel)
{
    for (size_t i = 0; i < m_listeningChannels.size(); ++i)
    {
        if (m_listeningChannels[i] == channel)
        {
            return true;
        }
    }

    return false;
}

// CConnectionBase

bool CNotificationNetwork::CConnection::OnMessage(EMessage eMessage, const CChannel& channel)
{
    switch (eMessage)
    {
    case eMessage_ChannelRegister:
        for (size_t i = 0; i < m_listeningChannels.size(); ++i)
        {
            if (m_listeningChannels[i] == channel)
            {
                return true;
            }
        }
        m_listeningChannels.push_back(channel);
        return true;

    case eMessage_ChannelUnregister:
        for (size_t i = 0; i < m_listeningChannels.size(); ++i)
        {
            if (m_listeningChannels[i] != channel)
            {
                continue;
            }

            m_listeningChannels[i] = m_listeningChannels.back();
            m_listeningChannels.pop_back();
            return true;
        }
        return true;
    }

    return false;
}

/*

  CNotificationNetwork::CThread

*/

CNotificationNetwork::CThread::CThread()
{
    m_pNotificationNetwork = nullptr;
    m_bRun = true;
}

CNotificationNetwork::CThread::~CThread()
{
}

//

bool CNotificationNetwork::CThread::Begin(CNotificationNetwork* pNotificationNetwork)
{
    m_pNotificationNetwork = pNotificationNetwork;
    Start(-1, (char*)NN_THREAD_NAME);
    return true;
}


void CNotificationNetwork::CThread::End()
{
    m_bRun = false;
    //  WaitForThread();

    // TODO: Should properly close!
}

// CryRunnable

void CNotificationNetwork::CThread::Run()
{
    CryThreadSetName(threadID(THREADID_NULL), NN_THREAD_NAME);
    while (m_bRun)
    {
        m_pNotificationNetwork->ProcessSockets();
    }
}

/*

  CNotificationNetwork

*/

CNotificationNetwork* CNotificationNetwork::Create()
{
    AZ::AzSock::Startup();

    AZSOCKET sock = AZ::AzSock::Socket();
    if (!AZ::AzSock::IsAzSocketValid(sock))
    {
        CryLog("CNotificationNetwork::Create: Failed to create socket.\n");
        return nullptr;
    }

    // Disable nagling of small blocks to fight high latency connection
    int result = AZ::AzSock::EnableTCPNoDelay(sock, true);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        AZ::AzSock::CloseSocket(sock);
        CryLog("CNotificationNetworkClient::Create: Failed to set TCP_NODELAY option.");
        return nullptr;
    }

    result = AZ::AzSock::SetSocketBlockingMode(sock, false);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        AZ::AzSock::CloseSocket(sock);
        CryLog("CNotificationNetworkClient::Connect: Failed to set socket to asynchronous operation.");
        return nullptr;
    }

    // Editor uses a different port to avoid conflicts when running both editor and game on same PC
    // But allows the lua remote debugger to connect to the editor
    unsigned short port = gEnv && gEnv->IsEditor() ? 9433 : 9432;

    AZ::AzSock::AzSocketAddress addr;
    addr.SetAddrPort(port);

    result = AZ::AzSock::Bind(sock, addr);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        CryLog("CNotificationNetwork::Create: Failed to bind socket.\n");
        AZ::AzSock::CloseSocket(sock);
        return nullptr;
    }

    result = AZ::AzSock::Listen(sock, 8);
    if (AZ::AzSock::SocketErrorOccured(result))
    {
        CryLog("CNotificationNetwork::Create: Failed to listen.\n");
        AZ::AzSock::CloseSocket(sock);
        return nullptr;
    }

    CNotificationNetwork* pNotificationNetwork = new CNotificationNetwork();
    pNotificationNetwork->m_socket = sock;

    pNotificationNetwork->m_thread.Begin(pNotificationNetwork);

    return pNotificationNetwork;
}

//

CNotificationNetwork::CNotificationNetwork()
{
    m_socket = AZ_SOCKET_INVALID;

    m_connections.reserve(4);

    m_listeners.Bind("Query", &g_queryNotification);
}

CNotificationNetwork::~CNotificationNetwork()
{
    m_thread.End();
    m_thread.Stop();
    m_thread.WaitForThread();
    while (!m_connections.empty())
    {
        delete m_connections.back();
        m_connections.pop_back();
    }

    if (m_socket != AZ_SOCKET_INVALID)
    {
        AZ::AzSock::CloseSocket(m_socket);
        m_socket = AZ_SOCKET_INVALID;
    }

    AZ::AzSock::Cleanup();
}

//

void CNotificationNetwork::ReleaseClients(CClient* pClient)
{
    // TODO: Use CryAutoLock
    LockDebug("Lock %s\n", "CNotificationNetwork::ReleaseClients()");
    m_clientsCriticalSection.Lock();
    for (size_t i = 0; i < m_clients.size(); ++i)
    {
        if (m_clients[i] != pClient)
        {
            continue;
        }

        m_clients[i] = m_clients.back();
        m_clients.pop_back();
        break;
    }
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock %s\n", "CNotificationNetwork::ReleaseClients()");
}

void CNotificationNetwork::ProcessSockets()
{
    fd_set read;
    FD_ZERO(&read);
    AZSOCKET socketMax = 0;
    if (m_socket != AZ_SOCKET_INVALID)
    {
        FD_SET(m_socket, &read);
        socketMax = m_socket;
    }
    for (size_t i = 0; i < m_connections.size(); ++i)
    {
        if (m_connections[i]->Validate())
        {
            AZSOCKET sock = m_connections[i]->GetSocket();
            FD_SET(sock, &read);

            if (socketMax < sock)
            {
                socketMax = sock;
            }

            continue;
        }

        // The Connection is invalid, remove it.
        CConnection* pConnection = m_connections[i];
        m_connections[i] = m_connections.back();
        m_connections.pop_back();
        delete pConnection;

        // Invalidate the loop increment since we just removed a Connection and
        // in the process potentially replaced its slot with an unprocessed one.
        --i;

        CryLog("Notification Network Connection terminated, current total: %d\n",
            (int)m_connections.size());
    }

    LockDebug("Lock %s\n", "CNotificationNetwork::ProcessSockets()");
    m_clientsCriticalSection.Lock();
    for (size_t i = 0; i < m_clients.size(); ++i)
    {
        if (!m_clients[i]->Validate())
        {
            continue;
        }

        AZSOCKET sock = m_clients[i]->GetSocket();
        FD_SET(sock, &read);

        if (socketMax < sock)
        {
            socketMax = sock;
        }
    }
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock %s\n", "CNotificationNetwork::ProcessSockets()");

    AZTIMEVAL timeOut = { 1, 0 };
    int r = AZ::AzSock::Select(socketMax, &read, nullptr, nullptr, &timeOut);
    if (r == 0)
    {
        return;
    }

    // When we have no sockets, the select statement will fail and not
    // block for even 1 second, as it should...
    if (AZ::AzSock::SocketErrorOccured(r))
    {
        // So we force the sleep here for now.
        Sleep(1000);
        return;
    }

    for (size_t i = 0; i < m_connections.size(); ++i)
    {
        if (!FD_ISSET(m_connections[i]->GetSocket(), &read))
        {
            continue;
        }

        m_connections[i]->Receive(m_listeners);
    }

    LockDebug("Lock 2 %s\n", "CNotificationNetwork::ProcessSockets()");
    m_clientsCriticalSection.Lock();
    for (size_t i = 0; i < m_clients.size(); ++i)
    {
        if (!FD_ISSET(m_clients[i]->GetSocket(), &read))
        {
            continue;
        }

        m_clients[i]->Receive();
    }
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock 2 %s\n", "CNotificationNetwork::ProcessSockets()");

    if (m_socket == AZ_SOCKET_INVALID)
    {
        return;
    }
    if (!FD_ISSET(m_socket, &read))
    {
        return;
    }

    AZ::AzSock::AzSocketAddress addr;
    AZSOCKET sock = AZ::AzSock::Accept(m_socket, addr);
    if (!AZ::AzSock::IsAzSocketValid(sock))
    {
        return;
    }

    if (!RCON_IsRemoteAllowedToConnect(addr))
    {
        AZ::AzSock::CloseSocket(sock);
        return;
    }

    m_connections.push_back(new CConnection(this, sock));

    CryLog("Notification Network accepted new Connection, current total: %d\n",
        (int)m_connections.size());
}

// INotificationNetwork

INotificationNetworkClient* CNotificationNetwork::CreateClient()
{
    CClient* pClient = CClient::Create(this);

    LockDebug("Lock %s\n", "CNotificationNetwork::CreateClient()");
    m_clientsCriticalSection.Lock();
    m_clients.push_back(pClient);
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock %s\n", "CNotificationNetwork::CreateClient()");

    return pClient;
}

INotificationNetworkClient* CNotificationNetwork::Connect(const char* address, uint16 port)
{
    CClient* pClient = CClient::Create(this, address, port);
    if (!pClient)
    {
        return nullptr;
    }

    LockDebug("Lock %s\n", "CNotificationNetwork::Connect()");
    m_clientsCriticalSection.Lock();
    m_clients.push_back(pClient);
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock %s\n", "CNotificationNetwork::Connect()");

    return pClient;
}

size_t CNotificationNetwork::GetConnectionCount(const char* channelName)
{
    if (!channelName)
    {
        return m_connections.size();
    }

    if (!CChannel::IsNameValid(channelName))
    {
        return 0;
    }

    CChannel channel(channelName);
    size_t count = 0;
    for (size_t i = 0; i < m_connections.size(); ++i)
    {
        if (!m_connections[i]->IsListening(channel))
        {
            continue;
        }

        ++count;
    }
    return count;
}

bool CNotificationNetwork::ListenerBind(const char* channelName, INotificationNetworkListener* pListener)
{
    if (!CChannel::IsNameValid(channelName))
    {
        return false;
    }

    return m_listeners.Bind(CChannel(channelName), pListener);
}

bool CNotificationNetwork::ListenerRemove(INotificationNetworkListener* pListener)
{
    return m_listeners.Remove(pListener);
}

void CNotificationNetwork::Update()
{
    m_listeners.NotificationsProcess();

    LockDebug("Lock %s\n", "CNotificationNetwork::Update()");
    m_clientsCriticalSection.Lock();
    for (size_t i = 0; i < m_clients.size(); ++i)
    {
        m_clients[i]->Update();
    }
    m_clientsCriticalSection.Unlock();
    LockDebug("Unlock %s\n", "CNotificationNetwork::Update()");
}

uint32 CNotificationNetwork::Send(const char* channelName, const void* pBuffer, size_t length)
{
    if (!CChannel::IsNameValid(channelName))
    {
        return 0;
    }

    CChannel channel(channelName);

    // TODO: There should be a mutex lock here to ensure thread safety.

    uint32 count = 0;
    for (size_t i = 0; i < m_connections.size(); ++i)
    {
        if (!m_connections[i]->IsListening(channel))
        {
            continue;
        }

        if (m_connections[i]->SendNotification(channel, pBuffer, length))
        {
            ++count;
        }
    }

    return count;
}
