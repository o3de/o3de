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

        void Clear();
        IndexType GetFreeSlotIndex();
        void RemoveIndex(IndexType index);
        void RemoveData(DataType* data);

        DataType& GetData(IndexType index);
        const DataType& GetData(IndexType index) const;
        size_t GetDataCount() const;

        AZStd::vector<DataType>& GetDataVector();
        const AZStd::vector<DataType>& GetDataVector() const;

        AZStd::vector<IndexType>& GetIndexVector();
        const AZStd::vector<IndexType>& GetIndexVector() const;

        IndexType GetRawIndex(IndexType index) const;
        IndexType GetIndexForData(const DataType* data) const;

    private:
        constexpr static size_t InitialReservedSize = 128;

        // Stores data indices and an embedded free list
        AZStd::vector<IndexType> m_indices;
        // Stores the indirection index
        AZStd::vector<IndexType> m_dataToIndices;
        AZStd::vector<DataType> m_data;
    };
} // namespace AZ::Render

#include <Atom/Feature/Utils/IndexedDataVector.inl>
