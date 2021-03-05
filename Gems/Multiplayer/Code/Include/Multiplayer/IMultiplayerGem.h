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

#include <AzCore/EBus/EBus.h>
#include <GridMate/Session/Session.h>
#include <GridMate/Carrier/DefaultSimulator.h>

namespace GridMate
{
    class SecureSocketDriver;
}

namespace Multiplayer
{
    class MultiplayerRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////


        //! Returns whether or not Network Security is enabled.
        virtual bool IsNetSecEnabled() const = 0;

        //! Returns whether or not Network Security is verifying the client
        virtual bool IsNetSecVerifyClient() const = 0;

        //! Enforces SecureSocketDriver over default(non-encrypted) socket driver. Works only for platforms supporting SecureSocketDriver (See NET_SUPPORT_SECURE_SOCKET_DRIVER compile option)
        virtual void RegisterSecureDriver(GridMate::SecureSocketDriver* driver) = 0;

        //! Retrieve current session
        virtual GridMate::GridSession* GetSession() = 0;

        //! Sets the current session
        virtual void RegisterSession(GridMate::GridSession* gridSession) = 0;

        //! Retrieve current network simulator or null if there is none
        virtual GridMate::Simulator* GetSimulator() = 0;

        //! Enable network simulator
        virtual void EnableSimulator() = 0;

        //! Disable network simulator
        virtual void DisableSimulator() = 0;        
    };
    using MultiplayerRequestBus = AZ::EBus<MultiplayerRequests>;

} // namespace Multiplayer
