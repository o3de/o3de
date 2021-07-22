/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QLocalSocket>
#endif

namespace AtomToolsFramework
{
    //! LocalSocket enables interprocess communication by establ;ishing a connection and sending data to a LocalServer
    class LocalSocket : public QObject
    {
        Q_OBJECT
    public:
        LocalSocket();
        ~LocalSocket();

        //! Attempt to connect to a named local server
        bool Connect(const QString& serverName);

        //! Sever connection from server
        void Disconnect();

        //! Get the sockets connection status
        bool IsConnected() const;

        //! Send a stream of data to the connected local server
        bool Send(const QByteArray& buffer);

    private:
        QString m_serverName;
        QLocalSocket m_socket;
    };
} // namespace AtomToolsFramework
