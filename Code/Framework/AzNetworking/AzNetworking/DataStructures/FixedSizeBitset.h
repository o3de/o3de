/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzNetworking/DataStructures/IBitset.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>

namespace AzNetworking
{
    //! @class FixedSizeBitset
    //! @brief fixed size data structure optimized for representing an array of bits.
    // @nt @KB - TODO: The default choice of uint32_t potentially has a negative impact on bandwidth usage, since a large number of components probably don't use 
    // multiples of 32 network properties i.e. 1 network property sends 4 bytes when it only needs to send 1, 33 network properties send 8 bytes when it only
    // needs to send 5. The more networked components we have, the larger this impact becomes. Consider optimizing this in the future
    template <AZStd::size_t SIZE, typename ElementType = uint32_t>
    class FixedSizeBitset
        : public IBitset
    {
    public:

        static const constexpr AZStd::size_t ElementTypeBits = sizeof(ElementType) * 8;
        static const constexpr AZStd::size_t ElementCount = (SIZE + ElementTypeBits - 1) / ElementTypeBits;

        using SelfType = FixedSizeBitset<SIZE, ElementType>;
        using ContainerType = AZStd::array<ElementType, ElementCount>;

        FixedSizeBitset();

        //! Construct to a given initial state.
        //! @param value value to initialize all bits to
        FixedSizeBitset(bool value);

        //! Construct from an array of raw input.
        //! @param values raw input array to initialize bit vector to
        FixedSizeBitset(const ElementType* values);

        //! Construct from another bit set.
        //! @param rhs bitset to copy
        FixedSizeBitset(const FixedSizeBitset<SIZE, ElementType> &rhs);

        //! Assignment from same type.
        //! @param rhs instance to assign from
        SelfType& operator =(const SelfType& rhs);

        //! Bitwise OR assignment operator.
        //! @param rhs instance to bitwise-or assign
        //! @return reference to the LHS
        SelfType& operator |=(const SelfType& rhs);

        //! Equality operator.
        //! @param rhs instance to compare against
        //! @return boolean true if inputs are the same, false otherwise
        bool operator ==(const SelfType& rhs) const;

        //! Inequality operator.
        //! @param rhs instance to compare against
        //! @return boolean true if inputs are different, false otherwise
        bool operator !=(const SelfType& rhs) const;

        //! Initializes all internal bits to the provided value.
        //! @param value value to initialize all bits to
        void InitializeAll(bool value);

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
        void Subtract(const FixedSizeBitset<SIZE, ElementType> &rhs);

        //! Const raw-buffer access.
        //! @return const pointer to the raw buffer the bit array is stored in
        const ContainerType &GetContainer() const;

        //! Non-const raw-buffer access.
        //! @return non-const pointer to the raw buffer the bit array is stored in
        ContainerType &GetContainer();

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer &serializer);

    private:

        void ClearUnusedBits();

        ContainerType m_container;

        template <AZStd::size_t, typename>
        friend class FixedSizeVectorBitset;
    };
}

#include <AzNetworking/DataStructures/FixedSizeBitset.inl>
