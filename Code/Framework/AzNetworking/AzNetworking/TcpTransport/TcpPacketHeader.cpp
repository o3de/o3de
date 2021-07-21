/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/TcpTransport/TcpPacketHeader.h>

namespace AzNetworking
{
    bool TcpPacketHeader::Serialize(ISerializer& serializer)
    {
        serializer.Serialize(m_packetFlags, "Flags");
        serializer.Serialize(m_packetType, "Type");
        serializer.Serialize(m_packetSize, "Size");
        return serializer.IsValid();
    }
}
