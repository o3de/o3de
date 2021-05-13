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

#include <Multiplayer/NetworkInput.h>

namespace Multiplayer
{
    //! Max number of entities that can be children of our netbound player entity.
    static constexpr uint32_t MaxEntityHierarchyChildren = 16;

    //! Used by the EntityHierarchyComponent.  This component allows the gameplay programmer to specify inputs for dependent entities.
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
}
