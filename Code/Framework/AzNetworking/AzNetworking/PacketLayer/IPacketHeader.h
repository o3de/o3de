/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    //! 
    //! IPacketHeader defines an abstract interface for a descriptor of all AzNetworking::IPacket sent through AzNetworking. The
    //! PacketHeader is used to identify and describe the contents of a Packet so that transport logic can identify what
    //! additional processing steps need to be taken (if any) and what type of Packet is being inspected.
    //! 
    //! The PacketFlags portion of the header represents the first byte of the header.  While it can be encrypted it is
    //! otherwise not exposed to additional processing (such as an AzNetworking::ICompressor).  PacketFlags are a bitfield use to provide up
    //! front information about the state of the packet. Currently there is only one flag to indicate if the Packet is
    //! compressed or not.
    //! 
    //! The remainder of the header contains the PacketType and the PacketId. While the PacketFlags byte is exempt from most
    //! additional forms of processing, the remainder of the header is not.
    
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
