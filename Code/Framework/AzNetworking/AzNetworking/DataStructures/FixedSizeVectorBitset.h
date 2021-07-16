/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/DataStructures/FixedSizeBitset.h>

namespace AzNetworking
{
    //! @class FixedSizeVectorBitset
    //! @brief fixed size data structure optimized for representing a resizable array of bits.
    template <AZStd::size_t CAPACITY, typename ElementType = uint8_t>
    class FixedSizeVectorBitset
        : public IBitset
    {
    public:

        using SelfType = FixedSizeVectorBitset<CAPACITY, ElementType>;

        FixedSizeVectorBitset() = default;

        //! Assignment from same type.
        //! @param rhs instance to assign from
        SelfType& operator =(const SelfType& rhs);

        //! Bitwise OR assignment operator.
        //! @param rhs instance to bitwise-or assign
        //! @return reference to the LHS
        SelfType& operator |=(const SelfType& rhs);

        //! Sets the specified bit to the provided value.
        //! @param index index of the bit to set
        //! @param value value to set the bit to
        void SetBit(uint32_t index, bool value) override;

        //! Gets the current value of the specified bit.
        //! @param index index of the bit to retrieve the value of
        //! @return boolean true if the bit is set, false otherwise
        bool GetBit(uint32_t index) const override;

        //! Returns true if any of the bits are set.
        //! @return boolean true if any bit is set, false otherwise
        bool AnySet() const override;

        //! Returns the number of bits that are represented in this fixed size bitset.
        //! @return the number of bits that are represented in this fixed size bitset
        uint32_t GetValidBitCount() const override;

        //! Subtracts off the set bits of the passed in bitset.
        //! @param rhs the bits that we want to remove from the current bitset
        void Subtract(const SelfType& rhs);

        uint32_t GetSize() const;
        uint32_t GetCapacity() const;

        bool Resize(uint32_t count);
        bool AddBits(uint32_t count);
        void Clear();

        bool GetBack() const;
        bool PushBack(bool value);
        bool PopBack();

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

    private:

        void ClearUnusedBits();

        using BitsetType = FixedSizeBitset<CAPACITY, ElementType>;
        uint32_t m_count = 0;
        BitsetType m_bitset = false;
    };
}

#include <AzNetworking/DataStructures/FixedSizeVectorBitset.inl>
