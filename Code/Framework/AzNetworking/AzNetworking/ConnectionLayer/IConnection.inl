/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline ConnectionQuality::ConnectionQuality(int32_t lossPercentage, AZ::TimeMs latencyMs, AZ::TimeMs varianceMs)
        : m_lossPercentage(lossPercentage)
        , m_latencyMs(latencyMs)
        , m_varianceMs(varianceMs)
    {
        ;
    }

    inline IConnection::IConnection(ConnectionId connectionId, const IpAddress& address)
        : m_connectionId(connectionId)
        , m_remoteAddress(address)
    {
        ;
    }

    inline ConnectionId IConnection::GetConnectionId() const
    {
        return m_connectionId;
    }

    inline void IConnection::SetUserData(void* userData)
    {
        m_userData = userData;
    }

    inline void* IConnection::GetUserData() const
    {
        return m_userData;
    }

    inline void IConnection::SetRemoteAddress(const IpAddress& address)
    {
        m_remoteAddress = address;
    }

    inline const IpAddress& IConnection::GetRemoteAddress() const
    {
        return m_remoteAddress;
    }

    inline const ConnectionMetrics& IConnection::GetMetrics() const
    {
        return m_connectionMetrics;
    }

    inline ConnectionMetrics& IConnection::GetMetrics()
    {
        return m_connectionMetrics;
    }
}
