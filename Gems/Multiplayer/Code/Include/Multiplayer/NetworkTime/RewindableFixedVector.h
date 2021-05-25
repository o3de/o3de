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

#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <Multiplayer/NetworkTime/RewindableObject.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    //! @class RewindableFixedVector
    //! @brief Data structure that has a compile-time upper bound, provides vector semantics and supports network serialization
    template <typename TYPE, uint32_t SIZE>
    class RewindableFixedVector
    {
    public:
        //! Default constructor
        RewindableFixedVector() = default;

        //! Construct and initialize buffer to the provided value
        //! @param initialValue initial value to set the internal buffer to
        //! @param count        initial value to reserve in the vector
        RewindableFixedVector(const TYPE& initialValue, uint32_t count);

        //! Destructor
        ~RewindableFixedVector();

        //! Serialization method for fixed vector contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

        //! Serialization method for fixed vector contained rewindable objects
        //! @param serializer ISerializer instance to use for serialization
        //! @return bool true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer, AzNetworking::IBitset &deltaRecord);

        //! Copies elements from the buffer pointed to by Buffer to this FixedSizeVector instance, vector size will be set to BufferSize
        //! @param buffer     pointer to the buffer to copy
        //! @param bufferSize number of elements in the buffer to copy
        //! @return bool true on success, false if the input data was too large to fit in the vector
        bool copy_values(const TYPE* buffer, uint32_t bufferSize);

        //! Copy buffer from the provided vector
        //! @param RHS instance to copy from
        RewindableFixedVector<TYPE, SIZE>& operator=(const RewindableFixedVector<TYPE, SIZE>& RHS);

        //! Equality operator, returns true if the current instance is equal to RHS
        //! @param RHS the FixedSizeVector instance to test for equality against
        //! @return bool true if equal, false if not
        bool operator ==(const RewindableFixedVector<TYPE, SIZE>& RHS) const;

        //! Inequality operator, returns true if the current instance is not equal to RHS
        //! @param RHS the FixedSizeVector instance to test for inequality against
        //! @return bool false if equal, true if not equal
        bool operator !=(const RewindableFixedVector<TYPE, SIZE>& RHS) const;

        //! Resizes the vector to the requested number of elements, initializing new elements if necessary
        //! @param count the number of elements to size the vector to
        //! @return bool true on success
        bool resize(uint32_t count);

        //! Resizes the vector to the requested number of elements, without initialization
        //! @param count the number of elements to size the vector to
        //! @return bool true on success
        bool resize_no_construct(uint32_t count);

        //! Resets the vector, returning it to size 0
        void clear();

        //! Const element access
        //! @param Index index of the element to return
        //! @return const reference to the requested element
        const TYPE& operator[](uint32_t index) const;

        //! Non-const element access
        //! @param Index index of the element to return
        //! @return non-const reference to the requested element
        TYPE& operator[](uint32_t index);

        //! Pushes a new element to the back of the vector
        //! @param Value value to append to the back of this vector
        //! @return boolean true on success, false if the vector was full
        bool push_back(const TYPE& value);

        //! Pops the last element off the vector, decreasing the vector's size by one
        //! @return bool true on success, false if the vector was empty
        bool pop_back();

        //! Returns if the vector is empty
        //! @return bool true on empty, false if the vector contains valid elements
        bool empty() const;

        //! Gets the last element of the vector
        const TYPE& back() const;

        //! Gets the size of the vector
        uint32_t size() const;

        typedef const RewindableObject<TYPE, Multiplayer::RewindHistorySize>* const_iterator;
        const_iterator begin() const { return m_container.cbegin(); }
        const_iterator end() const { return m_container.cend(); }
        typedef RewindableObject<TYPE, Multiplayer::RewindHistorySize>* iterator;
        iterator begin() { return m_container.begin(); }
        iterator end() { return m_container.end(); }

    private:
        AZStd::fixed_vector<RewindableObject<TYPE, Multiplayer::RewindHistorySize>, SIZE> m_container;
        // Synchronized value for vector size, prefer using size() locally which checks m_container.size()
        RewindableObject<uint32_t, Multiplayer::RewindHistorySize> m_size;
    };
}

#include <Multiplayer/NetworkTime/RewindableFixedVector.inl>
