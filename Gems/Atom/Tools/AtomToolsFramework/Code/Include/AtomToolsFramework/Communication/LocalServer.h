/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/functional.h>
#include <QByteArray>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>
#endif

namespace AtomToolsFramework
{
    //! A named local server that will manage connections and forward recieved data
    class LocalServer : public QObject
    {
        Q_OBJECT
    public:
        LocalServer();
        ~LocalServer();

        //! Start a named local server
        bool Connect(const QString& serverName);

        //! Stop the server
        void Disconnect();

        //! Get server status
        bool IsConnected() const;

        using ReadHandler = AZStd::function<void(const QByteArray&)>;

        //! Set a handler that recieved data will be forwarded to
        void SetReadHandler(ReadHandler handler);

    private:
        void AddConnection(QLocalSocket* connection);
        void ReadFromConnection(QLocalSocket* connection);
        void DeleteConnection(QLocalSocket* connection);

        QString m_serverName;
        QLocalServer m_server;
        ReadHandler m_readHandler;
    };
} // namespace AtomToolsFramework
