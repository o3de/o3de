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

#include <AzNetworking/DataStructures/ByteBuffer.h>


namespace Multiplayer
{
    static constexpr size_t GetMaxEditorServerInitSize()
    {
        constexpr size_t totalCapacity = AzNetworking::TcpPacketEncodingBuffer::GetCapacity();
        constexpr size_t packetOverhead = sizeof(bool) + 2 * sizeof(uint16_t); // m_lastUpdate + m_assetBuffer
        static_assert(totalCapacity > packetOverhead);
        return totalCapacity - packetOverhead;
    }
}
