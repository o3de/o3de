/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <stdint.h>

namespace AzNetworking
{
    //! @class IBitset
    //! @brief Interface for a structure optimized for representing an array of bits.
    class IBitset
    {
    public:

        virtual ~IBitset() = default;
        
        //! Sets the specified bit to the provided value.
        //! @param index index of the bit to set
        //! @param value value to set the bit to
        virtual void SetBit(uint32_t index, bool value) = 0;

        //! Gets the current value of the specified bit.
        //! @param index index of the bit to retrieve the value of
        //! @return boolean true if the bit is set, false otherwise
        virtual bool GetBit(uint32_t index) const = 0;

        //! Returns true if any bit is raised, false otherwise.
        //! @return boolean true if any bit is raised, false if all bits are lowered
        virtual bool AnySet() const = 0;

        //! Returns the number of bits that are represented in this fixed size bitset.
        //! @return the number of bits that are represented in this fixed size bitset
        virtual uint32_t GetValidBitCount() const = 0;
    };
}
