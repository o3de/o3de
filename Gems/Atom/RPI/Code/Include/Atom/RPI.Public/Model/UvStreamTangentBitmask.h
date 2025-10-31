/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <AzCore/base.h>
#include <limits.h>

namespace AZ
{
    namespace RPI
    {
        //! An encoded bitmask for tangent used by UV streams.
        //! It contains the information about number of UV streams and which tangent/bitangent is used by each UV stream.
        //! See m_mask for more details.
        //! The mask will be passed through per draw SRG.
        class ATOM_RPI_PUBLIC_API UvStreamTangentBitmask
        {
        public:
            //! Get the full mask including number of UVs and tangent/bitangent assignment to each UV.
            uint32_t GetFullTangentBitmask() const;

            //! Get number of UVs that have tangent/bitangent assigned.
            uint32_t GetUvStreamCount() const;

            //! Get tangent/bitangent assignment to the specified UV in the material.
            //! @param uvIndex the index of the UV from the material, in default order as in the shader code.
            uint32_t GetTangentAtUv(uint32_t uvIndex) const;

            //! Apply the tangent to the next UV, whose index is the same as GetUvStreamCount.
            //! @param tangent the tangent/bitangent to be assigned. Ranged in [0, 0xF)
            //!        It comes from the model in order, e.g. 0 means the first available tangent stream from the model.
            //!        Specially, value 0xF(=UnassignedTangent) means generated tangent/bitangent will be used in shader.
            //!        If ranged out of definition, unassigned tangent will be applied.
            void ApplyTangent(uint32_t tangent);

            //! Reset the bitmask to clear state.
            void Reset();

            //! The bit mask indicating generated tangent/bitangent will be used.
            static constexpr uint32_t UnassignedTangent = 0b1111u;

            //! The variable name defined in the SRG shader code.
            static constexpr const char* SrgName = "m_uvStreamTangentBitmask";
        private:
            //! Mask composition:
            //! The number of UV slots (highest 4 bits) + tangent mask (4 bits each) * 7
            //! e.g. 0x200000F0 means there are 2 UV streams,
            //!      the first UV stream uses 0th tangent stream (0x0),
            //!      the second UV stream uses the generated tangent stream (0xF).
            uint32_t m_mask = 0;

            //! Bit size in the mask composition.
            static constexpr uint32_t BitsPerTangent = 4;
            static constexpr uint32_t BitsForUvIndex = 4;

        public:
            //! Max UV slots available in this bit mask.
            static constexpr uint32_t MaxUvSlots = (sizeof(m_mask) * CHAR_BIT - BitsForUvIndex) / BitsPerTangent;
        };
    }
}
