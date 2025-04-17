/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

namespace AzNetworking
{
    using NetworkInterfaces = AZStd::unordered_map<AZ::Name, AZStd::unique_ptr<INetworkInterface>>;

    //! @class INetworking
    //! @brief The interface for creating and working with network interfaces.
    //!
    //! INetworking is an Az::Interface<T> that provides applications access to higher level networking abstractions.
    //! AzNetworking::INetworking can be used to instantiate new INetworkInterfaces that can be configured to operate over
    //! either TCP or UDP, enable or disable encryption, and be assigned a trust level.
    //! 
    //! INetworking is also responsible for registering ICompressorFactory implementations. This allows a developer to have
    //! access to multiple ICompressorFactory implementations by name.  The [MultiplayerCompressor
    //! Gem](http://o3de.org/docs/user-guide/gems/reference/multiplayer/multiplayer-compression) is an example of this using the
    //! [LZ4](https://wikipedia.org/wiki/LZ4_%28compression_algorithm%29) algorithm.
    //! 

    class INetworking
    {
    public:
        AZ_RTTI(INetworking, "{6E47367B-3AA5-4CB8-A691-4910168F287A}");

        virtual ~INetworking() = default;

        //! Creates a new network interface instance with the provided parameters.
        //! Caller does not assume ownership, instance should be destroyed by calling DestroyNetworkInterface
        //! @param name         the name to assign to this network interface
        //! @param protocolType the type of interface to instantiate (Tcp or Udp)
        //! @param trustZone    the trust level associated with this network interface (client to server or server to server)
        //! @param listener     the connection listener responsible for handling connection events
        //! @return pointer to the instantiated network interface, or nullptr on error
        virtual INetworkInterface* CreateNetworkInterface(const AZ::Name& name, ProtocolType protocolType, TrustZone trustZone, IConnectionListener& listener) = 0;

        //! Retrieves a network interface instance by name.
        //! @param name the name of the network interface to retrieve
        //! @return pointer to the requested network interface, or nullptr on error
        virtual INetworkInterface* RetrieveNetworkInterface(const AZ::Name& name) = 0;

        //! Destroys a network interface instance by name.
        //! @param name the name of the network interface to destroy
        //! @return boolean true on success or false on failure
        virtual bool DestroyNetworkInterface(const AZ::Name& name) = 0;

        //! Registers a Compressor Factory that can be used to create compressors for INetworkInterfaces
        //! Upon registry, INetworking assumes ownership of the factory pointer.
        //! @param factory The ICompressorFactory to register
        virtual void RegisterCompressorFactory(ICompressorFactory* factory) = 0;

        //! Creates a compressor using a registered factory looked up by name
        //! @param name The name of the Compressor Factory to use, must match result of factory->GetFactoryName()
        //! @return A unique_ptr to the new compressor
        virtual AZStd::unique_ptr<ICompressor> CreateCompressor(const AZStd::string_view name) = 0;

        //! Unregisters the compressor factory
        //! @param name The name of the Compressor factory to unregister, must match result of factory->GetFactoryName()
        //! @return Whether the factory was found and unregistered
        virtual bool UnregisterCompressorFactory(const AZStd::string_view name) = 0;

        //! Returns the raw network interfaces owned by the networking instance.
        //! @return the raw network interfaces owned by the networking instance
        virtual const NetworkInterfaces& GetNetworkInterfaces() const = 0;

        //! Returns the number of sockets monitored by our TcpListenThread.
        //! @return the number of sockets monitored by our TcpListenThread
        virtual uint32_t GetTcpListenThreadSocketCount() const = 0;

        //! Returns the total time spent updating our TcpListenThread.
        //! @return the total time spent updating our TcpListenThread
        virtual AZ::TimeMs GetTcpListenThreadUpdateTime() const = 0;

        //! Returns the number of sockets monitored by our UdpReaderThread.
        //! @return the number of sockets monitored by our UdpReaderThread
        virtual uint32_t GetUdpReaderThreadSocketCount() const = 0;

        //! Returns the total time spent updating our UdpReaderThread.
        //! @return the total time spent updating our UdpReaderThread
        virtual AZ::TimeMs GetUdpReaderThreadUpdateTime() const = 0;

        //! Forcibly swaps reader thread buffers and updates all Network Interfaces
        //! CAUTION: For use when SystemTickBus is suspended or similar
        virtual void ForceUpdate() = 0;
    };
}
