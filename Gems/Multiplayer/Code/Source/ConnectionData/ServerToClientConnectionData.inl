/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace Multiplayer
{
    inline bool ServerToClientConnectionData::CanSendUpdates() const
    {
        return m_canSendUpdates;
    }

    inline void ServerToClientConnectionData::SetCanSendUpdates(bool canSendUpdates)
    {
        m_canSendUpdates = canSendUpdates;
    }


    inline NetworkEntityHandle ServerToClientConnectionData::GetPrimaryPlayerEntity()
    {
        return m_controlledEntity;
    }

    inline const NetworkEntityHandle& ServerToClientConnectionData::GetPrimaryPlayerEntity() const
    {
        return m_controlledEntity;
    }
}
