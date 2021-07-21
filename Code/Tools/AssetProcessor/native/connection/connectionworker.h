/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QTcpSocket>
#include <QHostAddress>
#include "native/connection/connectionMessages.h"

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#endif

/** This Class is responsible for connecting to the client
 */

#undef SendMessage

namespace AssetProcessor
{
    class ConnectionWorker
        : public QObject
    {
        Q_OBJECT

    public:
        explicit ConnectionWorker(qintptr socketDescriptor = -1, QObject* parent = 0);
        virtual ~ConnectionWorker();
        QTcpSocket& GetSocket();
        void Reset();
        bool Terminate();

        bool ReadMessage(QTcpSocket& socket, AssetProcessor::Message& message);
        bool ReadData(QTcpSocket& socket, char* buffer, qint64 size);
        bool WriteMessage(QTcpSocket& socket, const AssetProcessor::Message& message);
        bool WriteData(QTcpSocket& socket, const char* buffer, qint64 size);

        //! True if we initiated the connection, false if someone connected to us.
        bool InitiatedConnection() const;

Q_SIGNALS:
        void ReceiveMessage(unsigned int type, unsigned int serial, QByteArray payload);
        void SocketIPAddress(QString ipAddress);
        void SocketPort(int port);
        void Identifier(QString identifier);
        void AssetPlatformsString(QString platform);
        void ConnectionDisconnected();
        void ConnectionEstablished(QString ipAddress, quint16 port);
        void ErrorMessage(QString msg);

        // the token identifies the unique connection instance, since multiple may have the same address
        void IsAddressInAllowedList(QHostAddress hostAddress, void* token); 

    public Q_SLOTS:
        void ConnectSocket(qintptr socketDescriptor);
        void ConnectToEngine(QString ipAddress, quint16 port);
        void EngineSocketHasData();
        void EngineSocketStateChanged(QAbstractSocket::SocketState socketState);
        void SendMessage(unsigned int type, unsigned int serial, QByteArray payload);
        void DisconnectSockets();
        void RequestTerminate();
        bool NegotiateDirect(bool initiate);

        // the token will be the same token sent in the allowedlisting request.
        void AddressIsInAllowedList(void* token, bool result);

    private Q_SLOTS:
        void TerminateConnection();
        

    private:
        QTcpSocket m_engineSocket;
        volatile bool m_terminate;
        volatile bool m_alreadySentTermination = false;
        bool m_initiatedConnection = false;
        bool m_engineSocketIsConnected = false;
        int m_waitDelay = 10000; //increased to 10000 as 5000 milliseconds was enough in the unloaded general case but when the computer is loaded we need more time to negotiate a connection or we only get connection failures 
    };
} // namespace AssetProcessor
