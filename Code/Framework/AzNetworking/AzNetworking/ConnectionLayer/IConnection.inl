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
