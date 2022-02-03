/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/NetworkInput/IMultiplayerComponentInput.h>
#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace Multiplayer
{
    // Forwards
    class NetBindComponent;

    //! @class NetworkInput
    //! @brief A single networked client input command.
    class NetworkInput final
    {
    public:
        //! Intentionally restrict instancing of these objects to associated containers classes only
        //! This is a mechanism used to restrict calling autonomous client predicted setter functions to the ProcessInput call chain only
        friend class NetworkInputArray;
        friend class NetworkInputMigrationVector;
        friend class NetworkInputHistory;
        friend class NetworkInputChild;

        NetworkInput(const NetworkInput&);
        NetworkInput& operator= (const NetworkInput&);

        void SetClientInputId(ClientInputId inputId);
        ClientInputId GetClientInputId() const;
        ClientInputId& ModifyClientInputId();

        void SetHostFrameId(HostFrameId hostFrameId);
        HostFrameId GetHostFrameId() const;
        HostFrameId& ModifyHostFrameId();

        void SetHostTimeMs(AZ::TimeMs hostTimeMs);
        AZ::TimeMs GetHostTimeMs() const;
        AZ::TimeMs& ModifyHostTimeMs();

        void SetHostBlendFactor(float hostBlendFactor);
        float GetHostBlendFactor() const;

        void AttachNetBindComponent(NetBindComponent* netBindComponent);

        bool Serialize(AzNetworking::ISerializer& serializer);

        const IMultiplayerComponentInput* FindComponentInput(NetComponentId componentId) const;
        IMultiplayerComponentInput* FindComponentInput(NetComponentId componentId);

        template <class InputType>
        const InputType* FindComponentInput() const
        {
            return static_cast<const InputType*>(FindComponentInput(InputType::s_netComponentId));
        }

        template <typename InputType>
        InputType* FindComponentInput()
        {
            return static_cast<InputType*>(FindComponentInput(InputType::s_netComponentId));
        }

    private:
        //! Only associated containers can instance; see above comments.
        NetworkInput() = default;
        void CopyInternal(const NetworkInput& rhs);

        MultiplayerComponentInputVector m_componentInputs;
        ClientInputId m_inputId = ClientInputId{ 0 };
        HostFrameId m_hostFrameId = InvalidHostFrameId;
        AZ::TimeMs m_hostTimeMs = AZ::Time::ZeroTimeMs;
        float m_hostBlendFactor = 0.f;
        ConstNetworkEntityHandle m_owner;
        bool m_wasAttached = false;
    };
}
