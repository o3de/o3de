/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
