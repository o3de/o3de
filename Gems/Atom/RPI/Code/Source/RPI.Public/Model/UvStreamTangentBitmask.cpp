/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>
#include <AzCore/Debug/Trace.h>

namespace AZ
{
    namespace RPI
    {
        uint32_t UvStreamTangentBitmask::GetFullTangentBitmask() const
        {
            return m_mask;
        }

        uint32_t UvStreamTangentBitmask::GetUvStreamCount() const
        {
            return m_mask >> (sizeof(m_mask) * CHAR_BIT - BitsForUvIndex);
        }

        uint32_t UvStreamTangentBitmask::GetTangentAtUv(uint32_t uvIndex) const
        {
            return (m_mask >> (BitsPerTangent * uvIndex)) & 0b1111u;
        }

        void UvStreamTangentBitmask::ApplyTangent(uint32_t tangentIndex)
        {
            uint32_t currentSlot = GetUvStreamCount();
            if (currentSlot >= MaxUvSlots)
            {
                AZ_Error("UV Stream", false, "Reaching the max of avaiblable stream slots.");
                return;
            }

            if (tangentIndex > UnassignedTangent)
            {
                AZ_Warning(
                    "UV Stream", false,
                    "Tangent index must use %d bits as defined in UvStreamTangentIndex::m_flag. Unassigned index will be applied.",
                    BitsPerTangent);
                tangentIndex = UnassignedTangent;
            }

            uint32_t clearMask = 0b1111u << (BitsPerTangent * currentSlot);
            clearMask = ~clearMask;

            // Clear the writing bits in case
            m_mask &= clearMask;

            // Write the bits to the slot
            m_mask |= (tangentIndex << (BitsPerTangent * currentSlot));

            // Increase the index
            m_mask += (1u << (sizeof(m_mask) * CHAR_BIT - BitsForUvIndex));
        }

        void UvStreamTangentBitmask::Reset()
        {
            m_mask = 0;
        }
    }
}
