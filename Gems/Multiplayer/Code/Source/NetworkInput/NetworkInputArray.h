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

#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace Multiplayer
{
    //! @class NetworkInputArray
    //! @brief An array of network inputs. Used to mitigate loss of input packets on the server. Compresses subsequent elements.
    class NetworkInputArray final
    {
    public:
        static constexpr uint32_t MaxElements = 8; // Never try to replicate a list larger than this amount

        NetworkInputArray();
        NetworkInputArray(const ConstNetworkEntityHandle& entityHandle);
        ~NetworkInputArray() = default;

        NetworkInput& operator[](uint32_t index);
        const NetworkInput& operator[](uint32_t index) const;

        bool Serialize(AzNetworking::ISerializer& serializer);

    private:

        struct Wrapper // Strictly a workaround to deal with the private constructor of NetworkInput
        {
            Wrapper() : m_networkInput() {}
            Wrapper(const NetworkInput& networkInput) : m_networkInput(networkInput) {}
            NetworkInput m_networkInput;
        };

        ConstNetworkEntityHandle m_owner;
        AZStd::array<Wrapper, MaxElements> m_inputs;
    };
}
