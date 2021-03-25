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

#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

namespace AzNetworking
{
    //! @class INetworking
    //! @brief The interface for creating and working with network interfaces.
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
        virtual INetworkInterface* CreateNetworkInterface(AZ::Name name, ProtocolType protocolType, TrustZone trustZone, IConnectionListener& listener) = 0;

        //! Retrieves a network interface instance by name.
        //! @param name the name of the network interface to retrieve
        //! @return pointer to the requested network interface, or nullptr on error
        virtual INetworkInterface* RetrieveNetworkInterface(AZ::Name name) = 0;

        //! Destroys a network interface instance by name.
        //! @param name the name of the network interface to destroy
        //! @return boolean true on success or false on failure
        virtual bool DestroyNetworkInterface(AZ::Name name) = 0;

        //! Registers a Compressor Factory that can be used to create compressors for INetworkInterfaces
        //! @param factory The ICompressorFactory to register
        virtual void RegisterCompressorFactory(ICompressorFactory* factory) = 0;

        //! Creates a compressor using a registered factory looked up by name
        //! @param name The name of the Compressor Factory to use, must match result of factory->GetFactoryName()
        //! @return A unique_ptr to the new compressor
        virtual AZStd::unique_ptr<ICompressor> CreateCompressor(AZ::Name name) = 0;

        //! Unregisters the compressor factory
        //! @param name The name of the Compressor factory to unregister, must match result of factory->GetFactoryName()
        //! @return Whether the factory was found and unregistered
        virtual bool UnregisterCompressorFactory(AZ::Name name) = 0;
    };
}
