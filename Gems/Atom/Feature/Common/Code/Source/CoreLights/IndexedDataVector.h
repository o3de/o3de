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
