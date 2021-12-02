/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/utilities/BatchApplicationServer.h>
#include "native/utilities/assetUtils.h"


BatchApplicationServer::BatchApplicationServer(QObject* parent)
    : ApplicationServer(parent)
{
}

BatchApplicationServer::~BatchApplicationServer()
{
}

bool BatchApplicationServer::startListening(unsigned short port)
{
    if (!isListening())
    {
        if (port == 0)
        {
            quint16 listeningPort = AssetUtilities::ReadListeningPortFromSettingsRegistry();
            m_serverListeningPort = static_cast<int>(listeningPort);

            // In batch mode, make sure we use a different port from the GUI
            ++m_serverListeningPort;
        }
        else
        {
            // override the port
            m_serverListeningPort = port;
        }

        // Since we're starting up builders ourselves and informing them of the port chosen, we can scan for a free port

        while (!listen(QHostAddress::Any, static_cast<quint16>(m_serverListeningPort)))
        {
            auto error = serverError();
            
            if (error == QAbstractSocket::AddressInUseError)
            {
                ++m_serverListeningPort;
            }
            else
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to start Asset Processor server.  Error: %s", errorString().toStdString().c_str());
                return false;
            }
        }
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Listening Port: %d\n", m_serverListeningPort);
        ApplicationServerBus::Handler::BusConnect();
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor server listening on port %d\n", m_serverListeningPort);
    }
    return true;
}


