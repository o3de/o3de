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

#include <Source/NetworkInput/IMultiplayerComponentInput.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace Multiplayer
{
    // Forwards
    class NetBindComponent;

    AZ_TYPE_SAFE_INTEGRAL(ClientInputId, uint16_t);

    //! @class NetworkInput
    //! @brief A single networked client input command.
    class NetworkInput final
    {
    public:
        //! Intentionally restrict instancing of these objects to associated containers classes only
        //! This is a mechanism used to restrict calling autonomous client predicted setter functions to the ProcessInput call chain only
        friend class NetworkInputVector;
        friend class MigrateNetworkInputVector;
        friend class NetworkInputHistory;
        friend class NetworkInputChild;

        NetworkInput(const NetworkInput&);
        NetworkInput& operator= (const NetworkInput&);

        void SetClientInputId(ClientInputId inputId);
        ClientInputId GetClientInputId() const;
        ClientInputId& ModifyClientInputId();

        void SetServerTimeMs(AZ::TimeMs serverTimeMs);
        AZ::TimeMs GetServerTimeMs() const;
        AZ::TimeMs& ModifyServerTimeMs();

        void AttachNetBindComponent(NetBindComponent* netBindComponent);

        bool Serialize(AzNetworking::ISerializer& serializer);

        const IMultiplayerComponentInput* FindComponentInput(NetComponentId componentId) const;
        IMultiplayerComponentInput* FindComponentInput(NetComponentId componentId);

        template <class InputType>
        const InputType* FindInput() const
        {
            return static_cast<const InputType*>(FindInput(InputType::s_Type));
        }

        template <typename InputType>
        InputType* FindInput()
        {
            return static_cast<InputType*>(FindInput(InputType::s_Type));
        }

    private:
        //! Only associated containers can instance; see above comments.
        NetworkInput() = default;
        void CopyInternal(const NetworkInput& rhs);

        MultiplayerComponentInputVector m_componentInputs;
        ClientInputId m_inputId = ClientInputId{ 0 };
        AZ::TimeMs m_serverTimeMs = AZ::TimeMs{ 0 };
        ConstNetworkEntityHandle m_owner;
        bool m_wasAttached = false;
    };
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(Multiplayer::ClientInputId);
