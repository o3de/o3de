/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    inline const AZStd::string& ClientToServerConnectionData::GetProviderTicket() const
    {
        return m_providerTicket;
    }

    inline void ClientToServerConnectionData::SetProviderTicket(const AZStd::string& ticket)
    {
        m_providerTicket = ticket;
    }

    inline bool ClientToServerConnectionData::DidHandshake() const
    {
        return m_didHandshake;
    }

    inline void ClientToServerConnectionData::SetDidHandshake(bool didHandshake)
    {
        m_didHandshake = didHandshake;
    }
}
