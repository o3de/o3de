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
#include <QTcpServer>
#include <native/utilities/AssetUtilEBusHelper.h>
#endif

/** This Class is responsible for listening and getting new connections
 */
class ApplicationServer
    : public QTcpServer,
    public ApplicationServerBus::Handler
{
    Q_OBJECT
public:
    explicit ApplicationServer(QObject* parent = 0);
    virtual ~ApplicationServer();
    void incomingConnection(qintptr socketDescriptor) override;
    virtual bool startListening(unsigned short port = 0) = 0;

    //! ApplicationServerBus Handler
    int GetServerListeningPort() const override;

    static const char RandomListeningPortOption[];

Q_SIGNALS:
    void newIncomingConnection(qintptr socketDescriptor);
    void ReadyToQuit(QObject* source);

public Q_SLOTS:
    void QuitRequested();


protected:
    int m_serverListeningPort = 0;

    bool m_isShuttingDown = false;
};
