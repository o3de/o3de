/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzNetworking/TcpTransport/TcpNetworkInterface.h>
#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzNetworking
{
    void NetworkingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetworkingSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void NetworkingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetworkingService"));
    }

    void NetworkingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetworkingService"));
    }

    NetworkingSystemComponent::NetworkingSystemComponent()
    {
        SocketLayerInit();
        EncryptionLayerInit();
        AZ::Interface<INetworking>::Register(this);

        m_listenThread = AZStd::make_unique<TcpListenThread>();
        m_readerThread = AZStd::make_unique<UdpReaderThread>();
    }

    NetworkingSystemComponent::~NetworkingSystemComponent()
    {
        // Delete all our network interfaces first so they can unregister from the reader and listen threads
        m_networkInterfaces.clear();

        m_compressorFactories.clear();

        m_readerThread = nullptr;
        m_listenThread = nullptr;

        AZ::Interface<INetworking>::Unregister(this);
        EncryptionLayerShutdown();
        SocketLayerShutdown();
    }

    void NetworkingSystemComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void NetworkingSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void NetworkingSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        AZ::TimeMs elapsedMs = aznumeric_cast<AZ::TimeMs>(aznumeric_cast<int64_t>(deltaTime / 1000.0f));
        m_readerThread->SwapBuffers();
        for (auto& networkInterface : m_networkInterfaces)
        {
            networkInterface.second->Update(elapsedMs);
        }
    }

    int NetworkingSystemComponent::GetTickOrder()
    {
        return AZ::TICK_PLACEMENT;
    }

    INetworkInterface* NetworkingSystemComponent::CreateNetworkInterface(AZ::Name name, ProtocolType protocolType, TrustZone trustZone, IConnectionListener& listener)
    {
        AZ_Assert(RetrieveNetworkInterface(name) == nullptr, "A network interface with this name already exists");

        AZStd::unique_ptr<INetworkInterface> result = nullptr;
        switch (protocolType)
        {
        case ProtocolType::Tcp:
            result = AZStd::make_unique<TcpNetworkInterface>(name, listener, trustZone, *m_listenThread);
            break;
        case ProtocolType::Udp:
            result = AZStd::make_unique<UdpNetworkInterface>(name, listener, trustZone, *m_readerThread);
            break;
        }
        INetworkInterface* returnResult = result.get();
        if (result != nullptr)
        {
            m_networkInterfaces.emplace(name, AZStd::move(result));
        }
        return returnResult;
    }

    INetworkInterface* NetworkingSystemComponent::RetrieveNetworkInterface(AZ::Name name)
    {
        auto networkInterface = m_networkInterfaces.find(name);
        if (networkInterface != m_networkInterfaces.end())
        {
            return networkInterface->second.get();
        }
        return nullptr;
    }

    bool NetworkingSystemComponent::DestroyNetworkInterface(AZ::Name name)
    {
        return m_networkInterfaces.erase(name) > 0;
    }

    void NetworkingSystemComponent::RegisterCompressorFactory(ICompressorFactory* factory)
    {
        AZ_Assert(m_compressorFactories.find(factory->GetFactoryName()) == m_compressorFactories.end(), "A compressor factory with this name already exists");

        m_compressorFactories.emplace(factory->GetFactoryName(), factory);
    }

    AZStd::unique_ptr<ICompressor> NetworkingSystemComponent::CreateCompressor(AZ::Name name)
    {
        auto compressorFactory = m_compressorFactories.find(name);
        if(compressorFactory != m_compressorFactories.end())
        {
            return compressorFactory->second->Create();
        }

        return nullptr;
    }

    bool NetworkingSystemComponent::UnregisterCompressorFactory(AZ::Name name)
    {
        return m_compressorFactories.erase(name) > 0;
    }

    const NetworkInterfaces& NetworkingSystemComponent::GetNetworkInterfaces() const
    {
        return m_networkInterfaces;
    }

    uint32_t NetworkingSystemComponent::GetTcpListenThreadSocketCount() const
    {
        return m_listenThread->GetSocketCount();
    }

    AZ::TimeMs NetworkingSystemComponent::GetTcpListenThreadUpdateTime() const
    {
        return m_listenThread->GetUpdateTimeMs();
    }

    uint32_t NetworkingSystemComponent::GetUdpReaderThreadSocketCount() const
    {
        return m_readerThread->GetSocketCount();
    }

    AZ::TimeMs NetworkingSystemComponent::GetUdpReaderThreadUpdateTime() const
    {
        return m_readerThread->GetUpdateTimeMs();
    }

    void NetworkingSystemComponent::DumpStats([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZLOG_INFO("Total sockets monitored by TcpListenThread: %u", GetTcpListenThreadSocketCount());
        AZLOG_INFO("Total time spent updating TcpListenThread: %lld", aznumeric_cast<AZ::s64>(GetTcpListenThreadUpdateTime()));
        AZLOG_INFO("Total sockets monitored by UdpReaderThread: %u", GetUdpReaderThreadSocketCount());
        AZLOG_INFO("Total time spent updating UdpReaderThread: %lld", aznumeric_cast<AZ::s64>(GetUdpReaderThreadUpdateTime()));

        for (auto& networkInterface : m_networkInterfaces)
        {
            const char* protocol = networkInterface.second->GetType() == ProtocolType::Tcp ? "Tcp" : "Udp";
            const char* trustZone = networkInterface.second->GetTrustZone() == TrustZone::ExternalClientToServer ? "ExternalClientToServer" : "InternalServerToServer";
            const uint32_t port = aznumeric_cast<uint32_t>(networkInterface.second->GetPort());
            AZLOG_INFO("%sNetworkInterface: %s - open to %s on port %u", protocol, networkInterface.second->GetName().GetCStr(), trustZone, port);

            const NetworkInterfaceMetrics& metrics = networkInterface.second->GetMetrics();
            AZLOG_INFO(" - Total time spent updating in milliseconds: %lld", aznumeric_cast<AZ::s64>(metrics.m_updateTimeMs));
            AZLOG_INFO(" - Total number of connections: %llu", aznumeric_cast<AZ::u64>(metrics.m_connectionCount));
            AZLOG_INFO(" - Total send time in milliseconds: %lld", aznumeric_cast<AZ::s64>(metrics.m_sendTimeMs));
            AZLOG_INFO(" - Total sent packets: %llu", aznumeric_cast<AZ::s64>(metrics.m_sendPackets));
            AZLOG_INFO(" - Total sent bytes after compression: %llu", aznumeric_cast<AZ::u64>(metrics.m_sendBytes));
            AZLOG_INFO(" - Total sent bytes before compression: %llu", aznumeric_cast<AZ::u64>(metrics.m_sendBytesUncompressed));
            AZLOG_INFO(" - Total sent compressed packets without benefit: %llu", aznumeric_cast<AZ::u64>(metrics.m_sendCompressedPacketsNoGain));
            AZLOG_INFO(" - Total gain from packet compression: %lld", aznumeric_cast<AZ::s64>(metrics.m_sendBytesCompressedDelta));
            AZLOG_INFO(" - Total packets resent: %llu", aznumeric_cast<AZ::u64>(metrics.m_resentPackets));
            AZLOG_INFO(" - Total receive time in milliseconds: %lld", aznumeric_cast<AZ::s64>(metrics.m_recvTimeMs));
            AZLOG_INFO(" - Total received packets: %llu", aznumeric_cast<AZ::u64>(metrics.m_recvPackets));
            AZLOG_INFO(" - Total received bytes after compression: %llu", aznumeric_cast<AZ::u64>(metrics.m_recvBytes));
            AZLOG_INFO(" - Total received bytes before compression: %llu", aznumeric_cast<AZ::u64>(metrics.m_recvBytesUncompressed));
            AZLOG_INFO(" - Total packets discarded due to load: %llu", aznumeric_cast<AZ::u64>(metrics.m_discardedPackets));
        }
    }
}
