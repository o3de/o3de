/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    inline PacketId MakePacketId(SequenceRolloverCount rolloverCount, SequenceId sequenceId)
    {
        return PacketId(uint16_t(sequenceId) | (static_cast<uint32_t>(uint16_t(rolloverCount)) << 16));
    }

    inline SequenceId ToSequenceId(PacketId packetId)
    {
        return SequenceId(uint32_t(packetId) & 0xFFFF); // just grab the first two bytes
    }

    inline SequenceRolloverCount ToRolloverCount(PacketId packetId)
    {
        return SequenceRolloverCount(uint32_t(packetId) >> 16); // shift out the sequence portion of the packet id
    }

    template <AZStd::size_t MAX_VALUE>
    inline constexpr auto GenerateIndexLabel(AZStd::size_t value)
    {
        constexpr AZStd::size_t NumHexDigits = AZ::RequiredBytesForValue<MAX_VALUE>() * 2;
        constexpr char NibbleTable[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

        AZStd::fixed_string<NumHexDigits + 1> result;
        result.resize_no_construct(NumHexDigits + 1);
        for (AZStd::size_t i = 0; i < NumHexDigits; ++i)
        {
            result[NumHexDigits - i - 1] = NibbleTable[value & 0x0F];
            value >>= 4;
        }
        result[NumHexDigits] = '\0'; // Guarantee null termination
        return result;
    }
}
