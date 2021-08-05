/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzNetworking/DataStructures/IBitset.h>
#include <AzNetworking/Serialization/ISerializer.h>

#include <Multiplayer/NetworkTime/RewindableObject.h>

namespace Multiplayer
{
    //! @class RewindableFixedVector
    //! @brief Data structure that has a compile-time upper bound, provides vector semantics and supports network serialization
    template <typename TYPE, uint32_t SIZE>
    class RewindableFixedVector
    {
    public:
        //! Default constructor
        constexpr RewindableFixedVector() = default;

        //! Construct and initialize buffer to the provided value
        //! @param initialValue initial value to set the internal buffer to
        //! @param count        initial value to reserve in the vector
        constexpr RewindableFixedVector(const TYPE& initialValue, uint32_t count);

        //! Destructor
        ~RewindableFixedVector();

        //! Serialization method for fixed vector contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

        //! Serialization method for fixed vector contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @param deltaRecord Bitset delta record used to detect state change during reconciliation
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer, AzNetworking::IBitset &deltaRecord);

        //! Copies elements from the buffer pointed to by Buffer to this FixedSizeVector instance, vector size will be set to BufferSize
        //! @param buffer     pointer to the buffer to copy
        //! @param bufferSize number of elements in the buffer to copy
        //! @return bool true on success, false if the input data was too large to fit in the vector
        constexpr bool copy_values(const TYPE* buffer, uint32_t bufferSize);

        //! Copy buffer from the provided vector
        //! @param RHS instance to copy from
        constexpr RewindableFixedVector<TYPE, SIZE>& operator=(const RewindableFixedVector<TYPE, SIZE>& rhs);

        //! Equality operator, returns true if the current instance is equal to RHS
        //! @param rhs the FixedSizeVector instance to test for equality against
        //! @return bool true if equal, false if not
        constexpr bool operator ==(const RewindableFixedVector<TYPE, SIZE>& rhs) const;

        //! Inequality operator, returns true if the current instance is not equal to RHS
        //! @param rhs the FixedSizeVector instance to test for inequality against
        //! @return bool false if equal, true if not equal
        constexpr bool operator !=(const RewindableFixedVector<TYPE, SIZE>& rhs) const;

        //! Resizes the vector to the requested number of elements, initializing new elements if necessary
        //! @param count the number of elements to size the vector to
        //! @return bool true on success
        constexpr bool resize(uint32_t count);

        //! Resizes the vector to the requested number of elements, without initialization
        //! @param count the number of elements to size the vector to
        //! @return bool true on success
        constexpr bool resize_no_construct(uint32_t count);

        //! Resets the vector, returning it to size 0
        constexpr void clear();

        //! Const element access
        //! @param Index index of the element to return
        //! @return const reference to the requested element
        constexpr const TYPE& operator[](uint32_t index) const;

        //! Non-const element access
        //! @param Index index of the element to return
        //! @return non-const reference to the requested element
        constexpr TYPE& operator[](uint32_t index);

        //! Pushes a new element to the back of the vector
        //! @param Value value to append to the back of this vector
        //! @return boolean true on success, false if the vector was full
        constexpr bool push_back(const TYPE& value);

        //! Pops the last element off the vector, decreasing the vector's size by one
        //! @return bool true on success, false if the vector was empty
        constexpr bool pop_back();

        //! Returns if the vector is empty
        //! @return bool true on empty, false if the vector contains valid elements
        constexpr bool empty() const;

        //! Gets the last element of the vector
        constexpr const TYPE& back() const;

        //! Gets the size of the vector
        constexpr uint32_t size() const;

        typedef const RewindableObject<TYPE, Multiplayer::RewindHistorySize>* const_iterator;
        const_iterator begin() const { return m_container.cbegin(); }
        const_iterator end() const { return m_container.cbegin() + aznumeric_cast<size_t>(size()); }
        typedef RewindableObject<TYPE, Multiplayer::RewindHistorySize>* iterator;
        constexpr iterator begin() { return m_container.begin(); }
        constexpr iterator end() { return m_container.begin() + aznumeric_cast<size_t>(size()); }

    private:
        AZStd::array<RewindableObject<TYPE, Multiplayer::RewindHistorySize>, SIZE> m_container;
        // Synchronized value for vector size, prefer using size() locally which checks m_container.size()
        RewindableObject<uint32_t, Multiplayer::RewindHistorySize> m_rewindableSize;
    };
}

#include <Multiplayer/NetworkTime/RewindableFixedVector.inl>
