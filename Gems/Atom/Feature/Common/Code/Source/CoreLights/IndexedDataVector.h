/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AtomCore/std/containers/array_view.h>

namespace AZ
{
    namespace Render
    {
        template <typename DataType, typename IndexType = uint16_t>
        class IndexedDataVector
        {
        public:
            IndexedDataVector();
            ~IndexedDataVector() = default;

            static constexpr IndexType NoFreeSlot = std::numeric_limits<IndexType>::max();
            IndexType m_firstFreeSlot = NoFreeSlot;

            void Clear();
            IndexType GetFreeSlotIndex();
            void RemoveIndex(IndexType index);
            
            DataType& GetData(IndexType index);
            const DataType& GetData(IndexType index) const;
            size_t GetDataCount() const;
            
            AZStd::vector<DataType>& GetDataVector();
            const AZStd::vector<DataType>& GetDataVector() const;

            IndexType GetRawIndex(IndexType index) const;

        private:

            static constexpr size_t InitialReservedCount = 128;

            // stores the index of data vector for respective light, it also include a linked list to flag the free slots
            AZStd::vector<IndexType> m_indices;
            // stores the index of index vector for respective light
            AZStd::vector<IndexType> m_dataToIndices;
            // stores light data
            AZStd::vector<DataType> m_data;
        };

#include "IndexedDataVector.inl"
    }
}
