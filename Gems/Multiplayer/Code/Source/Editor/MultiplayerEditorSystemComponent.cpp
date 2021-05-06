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

#include <Include/IMultiplayer.h>
#include <Include/IMultiplayerTools.h>
#include <Source/AutoGen/Multiplayer.AutoPackets.h>
#include <Source/MultiplayerSystemComponent.h>
#include <Source/Editor/MultiplayerEditorSystemComponent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>

namespace Multiplayer
{
    static const AZStd::string_view s_networkInterfaceName("MultiplayerEditorServerInterface");

    using namespace AzNetworking;

    AZ_CVAR(bool, editorsv_enabled, false, nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "Whether Editor launching a local server to connect to is supported");
    AZ_CVAR(AZ::CVarFixedString, editorsv_process, "", nullptr, AZ::ConsoleFunctorFlags::DontReplicate,
        "The server executable that should be run. Empty to use the current project's ServerLauncher");
    AZ_CVAR(AZ::CVarFixedString, sv_serveraddr, "127.0.0.1", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The address of the server to connect to");
    AZ_CVAR(uint16_t, sv_port, 30091, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port that the multiplayer editor gem will bind to for traffic");

    void MultiplayerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MultiplayerEditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void MultiplayerEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("MultiplayerService"));
    }

    void MultiplayerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MultiplayerEditorService"));
    }

    void MultiplayerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MultiplayerEditorService"));
    }

    MultiplayerEditorSystemComponent::MultiplayerEditorSystemComponent()
    {
        ;
    }

    void MultiplayerEditorSystemComponent::Activate()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        // Setup a network interface handled by MultiplayerSystemComponent
        if (m_editorNetworkInterface == nullptr)
        {
            AZ::Entity* systemEntity = this->GetEntity();
            MultiplayerSystemComponent* mpSysComponent = systemEntity->FindComponent<MultiplayerSystemComponent>();

            m_editorNetworkInterface = AZ::Interface<INetworking>::Get()->CreateNetworkInterface(
                AZ::Name(s_networkInterfaceName), ProtocolType::Tcp, TrustZone::ExternalClientToServer, *mpSysComponent);
        }
    }

    void MultiplayerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzFramework::GameEntityContextEventBus::Handler::BusDisconnect();
    }

    void MultiplayerEditorSystemComponent::NotifyRegisterViews()
    {
        AZ_Assert(m_editor == nullptr, "NotifyRegisterViews occurred twice!");
        m_editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(m_editor, &AzToolsFramework::EditorRequests::GetEditor);
        m_editor->RegisterNotifyListener(this);
    }

    void MultiplayerEditorSystemComponent::OnEditorNotifyEvent(EEditorNotifyEvent event)
    {
        switch (event)
        {
        case eNotify_OnQuit:
            AZ_Warning("Multiplayer Editor", m_editor != nullptr, "Multiplayer Editor received On Quit without an Editor pointer.");
            if (m_editor)
            {
                m_editor->UnregisterNotifyListener(this);
                m_editor = nullptr;
            }
            [[fallthrough]];
        case eNotify_OnEndGameMode:
            AZ::TickBus::Handler::BusDisconnect();
            // Kill the configured server if it's active
            if (m_serverProcess)
            {
                m_serverProcess->TerminateProcess(0);
                m_serverProcess = nullptr;
            }
            if (m_editorNetworkInterface)
            {
                // Disconnect the interface, connection management will clean it up
                m_editorNetworkInterface->Disconnect(m_editorConnId, AzNetworking::DisconnectReason::TerminatedByUser);
                m_editorConnId = AzNetworking::InvalidConnectionId;
            }
            break;
        }
    }

    void MultiplayerEditorSystemComponent::OnGameEntitiesStarted()
    {
        auto prefabEditorEntityOwnershipInterface = AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
        if (!prefabEditorEntityOwnershipInterface)
        {
            AZ_Error("MultiplayerEditor", prefabEditorEntityOwnershipInterface != nullptr, "PrefabEditorEntityOwnershipInterface unavailable");
        }
        const AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>>& assetData = prefabEditorEntityOwnershipInterface->GetPlayInEditorAssetData();
        
        AZStd::vector<uint8_t> buffer;
        AZ::IO::ByteContainerStream byteStream(&buffer);

        // Serialize Asset information and AssetData into a potentially large buffer
        for (auto asset : assetData)
        {
            AZ::Data::AssetId assetId = asset.GetId();
            AZ::Data::AssetType assetType = asset.GetType();
            const AZStd::string& assetHint = asset.GetHint();
            AZ::IO::SizeType assetHintSize = assetHint.size();
            AZ::Data::AssetLoadBehavior assetLoadBehavior = asset.GetAutoLoadBehavior();

            byteStream.Write(sizeof(AZ::Data::AssetId), reinterpret_cast<void*>(&assetId));
            byteStream.Write(sizeof(AZ::Data::AssetType), reinterpret_cast<void*>(&assetType));
            byteStream.Write(sizeof(assetHintSize), reinterpret_cast<void*>(&assetHintSize));
            byteStream.Write(assetHint.size(), assetHint.c_str());
            byteStream.Write(sizeof(AZ::Data::AssetLoadBehavior), reinterpret_cast<void*>(&assetLoadBehavior));

            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, asset.GetData(), asset.GetData()->GetType());
        }

        // BeginGameMode and Prefab Processing have completed at this point
        IMultiplayerTools* mpTools = AZ::Interface<IMultiplayerTools>::Get();
        if (editorsv_enabled && mpTools != nullptr && mpTools->DidProcessNetworkPrefabs())
        {
            AZ::TickBus::Handler::BusConnect();

            if (assetData.size() > 0)
            {
                // Assemble the server's path
                AZ::CVarFixedString serverProcess = editorsv_process;
                if (serverProcess.empty())
                {
                    // If enabled but no process name is supplied, try this project's ServerLauncher
                    serverProcess = AZ::Utils::GetProjectName() + ".ServerLauncher";
                }

                AZ::IO::FixedMaxPathString serverPath = AZ::Utils::GetExecutableDirectory();
                if (!serverProcess.contains(AZ_TRAIT_OS_PATH_SEPARATOR))
                {
                    // If only the process name is specified, append that as well
                    serverPath.append(AZ_TRAIT_OS_PATH_SEPARATOR + serverProcess);
                }
                else
                {
                    // If any path was already specified, then simply assign
                    serverPath = serverProcess;
                }

                if (!serverProcess.ends_with(AZ_TRAIT_OS_EXECUTABLE_EXTENSION))
                {
                    // Add this platform's exe extension if it's not specified
                    serverPath.append(AZ_TRAIT_OS_EXECUTABLE_EXTENSION);
                }

                // Start the configured server if it's available
                AzFramework::ProcessLauncher::ProcessLaunchInfo processLaunchInfo;
                processLaunchInfo.m_commandlineParameters = AZStd::string::format("\"%s\"", serverPath.c_str());
                processLaunchInfo.m_showWindow = true;
                processLaunchInfo.m_processPriority = AzFramework::ProcessPriority::PROCESSPRIORITY_NORMAL;

                m_serverProcess = AzFramework::ProcessWatcher::LaunchProcess(
                    processLaunchInfo, AzFramework::ProcessCommunicationType::COMMUNICATOR_TYPE_NONE);
            }
        }

        // Now that the server has launched, attempt to connect the NetworkInterface
        const AZ::CVarFixedString remoteAddress = sv_serveraddr;
        m_editorConnId = m_editorNetworkInterface->Connect(
            AzNetworking::IpAddress(remoteAddress.c_str(), sv_port, AzNetworking::ProtocolType::Tcp));

        // Read the buffer into EditorServerInit packets until we've flushed the whole thing
        byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        
        while (byteStream.GetCurPos() <  byteStream.GetLength())
        {     
            MultiplayerPackets::EditorServerInit packet;
            AzNetworking::TcpPacketEncodingBuffer& outBuffer = packet.ModifyAssetData();

            // Size the packet's buffer appropriately
            size_t readSize = TcpPacketEncodingBuffer::GetCapacity();
            size_t byteStreamSize = byteStream.GetLength() - byteStream.GetCurPos();
            if (byteStreamSize < readSize)
            {
                readSize = byteStreamSize;
            }

            outBuffer.Resize(readSize);
            byteStream.Read(readSize, outBuffer.GetBuffer());

            // If we've run out of buffer, mark that we're done
            if (byteStream.GetCurPos() == byteStream.GetLength())
            {
                packet.SetLastUpdate(true);
            }
            m_editorNetworkInterface->SendReliablePacket(m_editorConnId, packet);
        }

    }

    void MultiplayerEditorSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {

    }

    int MultiplayerEditorSystemComponent::GetTickOrder()
    {
        // Tick immediately after the network system component
        return AZ::TICK_PLACEMENT + 1;
    }
}
