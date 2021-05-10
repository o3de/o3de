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

#include <Source/NetworkInput/NetworkInput.h>
#include <Include/NetworkEntityHandle.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace Multiplayer
{
    //! @class NetworkInputMigrationVector
    //! @brief A variable sized array of input commands, used specifically when migrate a clients inputs.
    class NetworkInputMigrationVector final
    {
    public:
        static constexpr uint32_t MaxElements = 90; // Never try to migrate a list larger than this amount, bumped up to handle DTLS connection time

        NetworkInputMigrationVector();
        NetworkInputMigrationVector(const ConstNetworkEntityHandle& entityHandle);
        virtual ~NetworkInputMigrationVector() = default;

        uint32_t GetSize() const;
        NetworkInput& operator[](uint32_t index);
        const NetworkInput& operator[](uint32_t index) const;
        bool PushBack(const NetworkInput& networkInput);

        bool Serialize(AzNetworking::ISerializer& serializer);

    private:

        struct Wrapper // Strictly a workaround to deal with the private constructor of NetworkInput
        {
            Wrapper() : m_networkInput() {}
            Wrapper(const NetworkInput& networkInput) : m_networkInput(networkInput) {}
            bool Serialize(AzNetworking::ISerializer& serializer) { return m_networkInput.Serialize(serializer); }
            NetworkInput m_networkInput;
        };

        ConstNetworkEntityHandle m_owner;
        AZStd::fixed_vector<Wrapper, MaxElements> m_inputs;
    };
}
