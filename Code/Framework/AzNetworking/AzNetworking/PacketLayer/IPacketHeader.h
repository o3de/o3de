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

#include <AzCore/Preprocessor/Enum.h>
#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/DataStructures/FixedSizeBitset.h>
#include <AzNetworking/ConnectionLayer/ConnectionMetrics.h>

namespace AzNetworking
{
    AZ_ENUM_CLASS(PacketFlag
        , Compressed
        , MAX
    );
    using PacketFlagBitset = FixedSizeBitset<1, uint8_t>;
    static_assert(aznumeric_cast<int>(PacketFlag::MAX) <= 8, "PacketFlags are limited to 1 byte (8 flags)");

    //! @class IPacketHeader
    //! @brief A packet header that lets us deduce packet type for any incoming packet.
    class IPacketHeader
    {
    public:

        AZ_TYPE_INFO(IPacketHeader, "{90A0EFE3-01A4-4F04-87CF-E98E94D49648}");

        virtual ~IPacketHeader() = default;

        //! Returns the packet type.
        //! @return packet type
        virtual PacketType GetPacketType() const = 0;

        //! Returns the packet id.
        //! @return PacketId
        virtual PacketId GetPacketId() const = 0;

        //! Returns if the specified packet flag is set for this packet.
        //! @return true if the flag is set for this packet
        virtual bool IsPacketFlagSet(PacketFlag flag) const = 0;

        //! Sets the specified packet flag for this packet.
        virtual void SetPacketFlag(PacketFlag flag, bool value) = 0;
    };
}
