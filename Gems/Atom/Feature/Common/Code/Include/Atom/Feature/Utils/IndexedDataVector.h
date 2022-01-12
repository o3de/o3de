/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/std/containers/array_view.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Render
{
    // Growable vector that leverages indirection to support erasure of elements while maintaining
    // resident data in a densely packed region of memory. Useful as a backing store for growable
    // buffers intended to be uploaded to the GPU for example.
    template<typename DataType, typename IndexType = uint16_t>
    class IndexedDataVector
    {
    public:
        IndexedDataVector();
        explicit IndexedDataVector(size_t initialReservedSize);
        ~IndexedDataVector() = default;

        static constexpr IndexType NoFreeSlot = std::numeric_limits<IndexType>::max();
        IndexType m_firstFreeSlot = NoFreeSlot;

        //! Clears all data and resets to initial state.
        void Clear();

        //! Creates a new entry, default-constructs it, and returns an index that references it.
        IndexType GetFreeSlotIndex();

        //! Destroys the data referenced by index and frees that index for future use.
        void RemoveIndex(IndexType index);

        //! Destroys the data and related index by using a pointer to the data itself.
        void RemoveData(DataType* data);

        //! Returns a reference to the data using the provided index.
        DataType& GetData(IndexType index);
        const DataType& GetData(IndexType index) const;

        //! Returns a count of how many items are stored in the IndexedDataVector
        size_t GetDataCount() const;

        //! Returns a reference to the internal data vector.
        //! This vector should not be altered by calling code or the IndexedDataVector will be corrupted
        AZStd::vector<DataType>& GetDataVector();
        const AZStd::vector<DataType>& GetDataVector() const;
        
        //! Returns a reference to the internal vector.
        const AZStd::vector<IndexType>& GetDataToIndexVector() const;

        //! Returns the offset into the internal data vector for a given index.
        IndexType GetRawIndex(IndexType index) const;

        //! Returns the logical index for data given its pointer, which could passed to
        //! GetData() to retrieve the data again.
        IndexType GetIndexForData(const DataType* data) const;

    private:
        constexpr static size_t InitialReservedSize = 128;

        // Indices to data and an embedded free list in the unused entries
        AZStd::vector<IndexType> m_indices;

        // Map of the physical index in m_data to the logical index for that data in m_indices.
        AZStd::vector<IndexType> m_dataToIndices;

        // The actual data.
        AZStd::vector<DataType> m_data;
    };
} // namespace AZ::Render

#include <Atom/Feature/Utils/IndexedDataVector.inl>
