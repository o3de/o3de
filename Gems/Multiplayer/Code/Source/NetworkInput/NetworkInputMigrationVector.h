/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
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
