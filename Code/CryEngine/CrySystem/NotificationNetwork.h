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

#ifndef CRYINCLUDE_CRYSYSTEM_NOTIFICATIONNETWORK_H
#define CRYINCLUDE_CRYSYSTEM_NOTIFICATIONNETWORK_H

#pragma once

#include <IConsole.h>
#include <INotificationNetwork.h>
#include <CryThread.h>

#include <AzCore/Socket/AzSocket_fwd.h>

class CNotificationNetwork;
namespace NotificationNetwork {
    // Constants

    static const uint32 NN_PACKET_HEADER_LENGTH = 2 * sizeof(uint32) + NN_CHANNEL_NAME_LENGTH_MAX;

    static const uint32 NN_PACKET_HEADER_OFFSET_MESSAGE = 0;
    static const uint32 NN_PACKET_HEADER_OFFSET_DATA_LENGTH = sizeof(uint32);
    static const uint32 NN_PACKET_HEADER_OFFSET_CHANNEL = sizeof(uint32) + sizeof(uint32);

    static const char* NN_THREAD_NAME = "NotificationNetwork";

    enum EMessage
    {
        eMessage_DataTransfer       = 0xbada2217,

        eMessage_ChannelRegister    = 0xab4eda30,
        eMessage_ChannelUnregister  = 0xfa4e3423,
    };

    // Classes

    struct CChannel
    {
    public:
        static bool IsNameValid(const char* name);

    public:
        CChannel();
        CChannel(const char* name);
        ~CChannel();

    public:
        void WriteToPacketHeader(void* pPacket) const;
        void ReadFromPacketHeader(void* pPacket);

    public:
        bool operator ==(const CChannel& channel) const;
        bool operator !=(const CChannel& channel) const;

    private:
        char m_name[NN_CHANNEL_NAME_LENGTH_MAX];
    };

    // TEMP
    struct SBuffer
    {
        uint8* pData;
        uint32 length;
        CChannel channel;
    };

    class CListeners
    {
    public:
        CListeners();
        ~CListeners();

    public:
        size_t Count() { return m_listeners.size(); }
        size_t Count(const CChannel& channel);

        CChannel& Channel(size_t index) { return m_listeners[index].second; }
        CChannel* Channel(INotificationNetworkListener* pListener);

        bool Bind(const CChannel& channel, INotificationNetworkListener* pListener);
        bool Remove(INotificationNetworkListener* pListener);

        void NotificationPush(const SBuffer& buffer);
        void NotificationsProcess();

    private:
        std::vector< std::pair<INotificationNetworkListener*, CChannel> > m_listeners;

        std::queue<SBuffer> m_notifications[2];
        std::queue<SBuffer>* m_pNotificationWrite;
        std::queue<SBuffer>* m_pNotificationRead;
        CryCriticalSection m_notificationCriticalSection;
    };

    class CConnectionBase
    {
    public:
        CConnectionBase(CNotificationNetwork* pNotificationNetwork);
        virtual ~CConnectionBase();

    public:
        AZSOCKET CreateSocket();

        bool Connect(const char* address, uint16 port);

        AZSOCKET GetSocket() { return m_socket; }

        bool Validate();

        bool SendNotification(const CChannel& channel, const void* pBuffer, size_t length);

        bool Receive(CListeners& listeners);

        bool GetIsConnectedFlag();
        bool GetIsFailedToConnectFlag() const;

    protected:
        CNotificationNetwork* GetNotificationNetwork() { return m_pNotificationNetwork; }

        void SetAddress(const char* address, uint16 port);
        void SetSocket(AZSOCKET sock) { m_socket = sock; }

        bool Send(const void* pBuffer, size_t length);
        bool SendMessage(EMessage eMessage, const CChannel& channel, uint32 data);

        bool Select_Internal();
        void CloseSocket_Internal();

        virtual bool OnConnect([[maybe_unused]] bool bConnectionResult) { return true; }
        virtual bool OnDisconnect() {return true; }
        virtual bool OnMessage([[maybe_unused]] EMessage eMessage, [[maybe_unused]] const CChannel& channel) { return false; }

    private:
        bool ReceiveMessage(CListeners& listeners);
        bool ReceiveNotification(CListeners& listeners);

    protected:
        CNotificationNetwork* m_pNotificationNetwork;

        char m_address[16];
        uint16 m_port;

        AZSOCKET m_socket;

        uint8 m_bufferHeader[NN_PACKET_HEADER_LENGTH];
        SBuffer m_buffer;
        uint32 m_dataLeft;

        volatile bool       m_boIsConnected;
        volatile bool   m_boIsFailedToConnect;
    };

    class CClient
        : public CConnectionBase
        , public INotificationNetworkClient
    {
    public:
        typedef std::vector<INotificationNetworkConnectionCallback*>    TDNotificationNetworkConnectionCallbacks;

        static CClient* Create(CNotificationNetwork* pNotificationNetwork, const char* address, uint16 port);
        static CClient* Create(CNotificationNetwork* pNotificationNetwork);

    private:
        CClient(CNotificationNetwork* pNotificationNetwork);
        ~CClient();

    public:
        bool Receive() { return CConnectionBase::Receive(m_listeners); }

        void Update();

        // CConnectionBase
    public:
        virtual bool OnConnect(bool bConnectionResult);
        virtual bool OnDisconnect();
        virtual bool OnMessage(EMessage eMessage, const CChannel& channel);

        // INotificationNetworkClient
    public:
        bool Connect(const char* address, uint16 port);

        void Release() { delete this; }

        virtual bool ListenerBind(const char* channelName, INotificationNetworkListener* pListener);
        virtual bool ListenerRemove(INotificationNetworkListener* pListener);

        virtual bool Send(const char* channelName, const void* pBuffer, size_t length);

        virtual bool IsConnected() {return CConnectionBase::GetIsConnectedFlag(); }
        virtual bool IsFailedToConnect() const{return CConnectionBase::GetIsFailedToConnectFlag(); }

        virtual bool RegisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback);
        virtual bool UnregisterCallbackListener(INotificationNetworkConnectionCallback* pConnectionCallback);
    private:
        CListeners m_listeners;

        TDNotificationNetworkConnectionCallbacks    m_cNotificationNetworkConnectionCallbacks;
        CryCriticalSection                                              m_stConnectionCallbacksLock;
    };
} // namespace NotificationNetwork
class CNotificationNetwork
    : public INotificationNetwork
{
private:
    class CConnection
        : public NotificationNetwork::CConnectionBase
    {
    public:
        CConnection(CNotificationNetwork* pNotificationNetwork, AZSOCKET sock);
        virtual ~CConnection();

    public:
        bool IsListening(const NotificationNetwork::CChannel& channel);

        // CConnectionBase
    protected:
        virtual bool OnMessage(NotificationNetwork::EMessage eMessage, const NotificationNetwork::CChannel& channel);

    private:
        std::vector<NotificationNetwork::CChannel> m_listeningChannels;
    };

    class CThread
        : public CryThread<CThread>
    {
    public:
        CThread();
        ~CThread();

    public:
        bool Begin(CNotificationNetwork* pNotificationNetwork);
        void End();

        // CryRunnable
    public:
        virtual void Run();

    private:
        CNotificationNetwork* m_pNotificationNetwork;
        bool m_bRun;
    } m_thread;

public:
    static CNotificationNetwork* Create();

public:
    CNotificationNetwork();
    ~CNotificationNetwork();

public:
    void ReleaseClients(NotificationNetwork::CClient* pClient);

private:
    void ProcessSockets();

    // INotificationNetwork
public:
    virtual void Release() { delete this; }

    virtual INotificationNetworkClient* CreateClient();

    virtual INotificationNetworkClient* Connect(const char* address, uint16 port);

    virtual size_t GetConnectionCount(const char* channelName);

    virtual void Update();

    virtual bool ListenerBind(const char* channelName, INotificationNetworkListener* pListener);
    virtual bool ListenerRemove(INotificationNetworkListener* pListener);

    virtual uint32 Send(const char* channelName, const void* pBuffer, size_t length);

private:
    AZSOCKET m_socket;

    std::vector<CConnection*> m_connections;
    std::vector<NotificationNetwork::CClient*> m_clients;
    NotificationNetwork::CListeners m_listeners;

    CryCriticalSection m_clientsCriticalSection;
};

#endif // CRYINCLUDE_CRYSYSTEM_NOTIFICATIONNETWORK_H
