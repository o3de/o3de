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
