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
#ifndef UNITTESTSHADERCOMPILERSERVER_H
#define UNITTESTSHADERCOMPILERSERVER_H

#if !defined(Q_MOC_RUN)
#include <QString>
#include <QByteArray>
#include <QObject>
#endif

class QTcpServer;
class QTcpSocket;
/** This Class will be used by the UnitTest class for testing
 * the Shader Compiler Manager Class.Basically we change the value
 * of the server status variable and see whether we are getting the
 * expected response from the shader compiler.
 */
class UnitTestShaderCompilerServer
    : public QObject
{
    Q_OBJECT
public:
    explicit UnitTestShaderCompilerServer(QObject* parent = 0);
    virtual ~UnitTestShaderCompilerServer();
    enum ServerStatus
    {
        GoodServer,
        BadServer_SendsIncompletePayload,
        BadServer_ReadsPayloadAndDisconnect,
        BadServer_DisconnectAfterConnect,
    };
    void constructPayload(QByteArray& payload);
    void Init(QString serverAddress, int serverPort);
    void closeSocket();
    void startServer();
    void setServerStatus(const ServerStatus& serverStatus);

signals:
    void errorMessage(QString error);


public slots:
    void newConnection();
    void incomingMessage();


private:
    QTcpSocket* m_socket;
    QTcpServer* m_server;
    ServerStatus m_serverStatus;
    QString m_serverAddress;
    int m_serverPort;
    QString m_incomingPayload;
    QString m_outgoingPayload;
    bool m_isPayloadSizeKnown;
    qint64 m_payloadSize;
    qint64 m_bytesRemainingInPayloadSize;
    qint64 m_totalBytesReadInPayloadSize;
    QByteArray m_payload;
    qint64 m_bytesRemainingInPayload;
    qint64 m_totalBytesReadInPayload;
};
#endif //UNITTESTSHADERCOMPILERSERVER_H
