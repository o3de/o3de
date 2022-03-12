/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkInput/NetworkInput.h>

namespace Multiplayer
{
    //! Used by the NetworkHierarchyRootComponent.  This component allows the gameplay programmer to specify inputs for dependent entities.
    //! Since it is possible to for the Client/Server to disagree about the state of related entities,
    //! this network input encodes the entity that is associated with it.
    class NetworkInputChild
    {
    public:
        NetworkInputChild() = default;
        NetworkInputChild(const NetworkInputChild& rhs) = default;
        NetworkInputChild(const ConstNetworkEntityHandle& entityHandle);

        NetworkInputChild& operator= (const NetworkInputChild& rhs);

        void Attach(const ConstNetworkEntityHandle& entityHandle);
        const ConstNetworkEntityHandle& GetOwner() const;
        const NetworkInput& GetNetworkInput() const;
        NetworkInput& GetNetworkInput();

        bool Serialize(AzNetworking::ISerializer& serializer);
    private:
        ConstNetworkEntityHandle m_owner;
        NetworkInput m_networkInput;
    };

    using NetworkInputChildList = AZStd::vector<NetworkInputChild>;
}
