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
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Cannot start Asset Processor server - another instance of the Asset Processor may already be running on port number %d.  If you'd like to run multiple Asset Processors on different branches at the same time, please edit bootstrap.cfg and assign different remote_port values to each branch instance.\n", m_serverListeningPort);
            return false;
        }
        m_serverListeningPort = serverPort();
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Listening Port: %d\n", m_serverListeningPort);
        ApplicationServerBus::Handler::BusConnect();
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Asset Processor server listening on port %d\n", m_serverListeningPort);
    }
    return true;
}


