/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/TcpTransport/TcpListenThread.h>
#include <AzNetworking/UdpTransport/UdpHeartbeatThread.h>
#include <AzNetworking/UdpTransport/UdpReaderThread.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzNetworking
{
    //! Implementation of the networking system interface.
    //! This class creates and manages the set of network interfaces used by the application.
    class NetworkingSystemComponent final
        : public AZ::Component
        , public AZ::SystemTickBus::Handler
        , public INetworking
    {
    public:
        AZ_COMPONENT(NetworkingSystemComponent, "{29914D25-5E8F-49C9-8C57-5125ABD3D489}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetworkingSystemComponent();
        ~NetworkingSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! AZ::SystemTickBus::Handler overrides.
        //! @{
        void OnSystemTick() override;
        //! @}

        //! INetworking overrides.
        //! @{
        INetworkInterface* CreateNetworkInterface(const AZ::Name& name, ProtocolType protocolType, TrustZone trustZone, IConnectionListener& listener) override;
        INetworkInterface* RetrieveNetworkInterface(const AZ::Name& name) override;
        bool DestroyNetworkInterface(const AZ::Name& name) override;
        void RegisterCompressorFactory(ICompressorFactory* factory) override;
        AZStd::unique_ptr<ICompressor> CreateCompressor(const AZStd::string_view name) override;
        bool UnregisterCompressorFactory(const AZStd::string_view name) override;
        const NetworkInterfaces& GetNetworkInterfaces() const override;
        uint32_t GetTcpListenThreadSocketCount() const override;
        AZ::TimeMs GetTcpListenThreadUpdateTime() const override;
        uint32_t GetUdpReaderThreadSocketCount() const override;
        AZ::TimeMs GetUdpReaderThreadUpdateTime() const override;
        void ForceUpdate() override;
        //! @}

        //! Console commands.
        //! @{
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        //! @}

    private:

        AZ_CONSOLEFUNC(NetworkingSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dumps stats for all instantiated network interfaces");

        NetworkInterfaces m_networkInterfaces;
        AZStd::unique_ptr<TcpListenThread> m_listenThread;
        AZStd::unique_ptr<UdpReaderThread> m_readerThread;
        AZStd::unique_ptr<UdpHeartbeatThread> m_heartbeatThread;

        using CompressionFactories = AZStd::unordered_map<AZ::Crc32, AZStd::unique_ptr<ICompressorFactory>>;
        CompressionFactories m_compressorFactories;
    };
}
