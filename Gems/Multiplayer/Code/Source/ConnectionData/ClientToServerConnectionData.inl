/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace Multiplayer
{
    inline bool ClientToServerConnectionData::CanSendUpdates() const
    {
        return m_canSendUpdates;
    }

    inline void ClientToServerConnectionData::SetCanSendUpdates(bool canSendUpdates)
    {
        m_canSendUpdates = canSendUpdates;
    }
}
