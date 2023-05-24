/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <native/utilities/GUIApplicationServer.h>
#include <AzFramework/API/ApplicationAPI.h>
#include "native/utilities/assetUtils.h"


GUIApplicationServer::GUIApplicationServer(QObject* parent)
    : ApplicationServer(parent)
{
}

GUIApplicationServer::~GUIApplicationServer()
{
}

bool GUIApplicationServer::startListening(unsigned short port)
{
    if (!isListening())
    {
        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);
        bool randomPort = commandLine && commandLine->HasSwitch(ApplicationServer::RandomListeningPortOption);
        if (port == 0 && !randomPort)
        {
            quint16 listeningPort = AssetUtilities::ReadListeningPortFromSettingsRegistry();
            m_serverListeningPort = static_cast<int>(listeningPort);
        }
        else
        {
            // override the port
            m_serverListeningPort = port;
        }

        if (!listen(QHostAddress::Any, aznumeric_cast<quint16>(m_serverListeningPort)))
        {
            AZ_Error(
                AssetProcessor::ConsoleChannel,
                false,
                "Cannot start Asset Processor server - another instance of the Asset Processor may already be running on port number %d.  "
                "If you'd like to run multiple Asset Processors on different branches at the same time, please modify the /Amazon/AzCore/Bootstrap/remote_port "
                " registry setting (by default this is set in bootstrap.setreg) and assign different remote_port values to each branch "
                "instance.\n",
                m_serverListeningPort);
            return false;
        }
        m_serverListeningPort = serverPort();
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Listening Port: %d\n", m_serverListeningPort);
        ApplicationServerBus::Handler::BusConnect();
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor server listening on port %d\n", m_serverListeningPort);
    }
    return true;
}


