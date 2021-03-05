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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QList>
#include <QAbstractSocket>
#endif

class QTcpSocket;
class QTcpServer;
class ApplicationManagerBase;

/** This Class is responsible for listening and getting new connections and
*   responding to text queries and commands from the socket.  The original purpose
*   is to enable writing more reliable and better performing tests which launch
*   AP as a subprocess such as our python test modules.
 */
class ControlRequestHandler : public QObject
{
    Q_OBJECT
public:
    explicit ControlRequestHandler(ApplicationManagerBase* parent = 0);
    ~ControlRequestHandler();

public slots:
    void GotConnection();
    void SocketStateUpdate(QAbstractSocket::SocketState newSocketState);
    void DataReceived();
    void Disconnected();
    void AssetManagerIdleStateChange(bool isIdle);

protected:
    bool StartListening(unsigned short port = 0);
    void ReadData(QTcpSocket* incoming);

private:
    QList<QTcpSocket*> m_listenSockets;
    QList<QTcpSocket*> m_idleWaitSockets;
    QTcpServer* m_tcpServer{ nullptr };
    ApplicationManagerBase* m_applicationManager{ nullptr };
};
