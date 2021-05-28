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
