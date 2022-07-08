/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::Render
{
    template<typename DataType, typename IndexType>
    inline IndexedDataVector<DataType, IndexType>::IndexedDataVector()
        : IndexedDataVector(InitialReservedSize)
    {
    }

    template<typename DataType, typename IndexType>
    inline IndexedDataVector<DataType, IndexType>::IndexedDataVector(size_t initialReservedSize)
    {
        m_dataToIndices.reserve(initialReservedSize);
        m_indices.reserve(initialReservedSize);
        m_data.reserve(initialReservedSize);
    }

    template<typename DataType, typename IndexType>
    inline void IndexedDataVector<DataType, IndexType>::Clear()
    {
        m_dataToIndices.clear();
        m_indices.clear();
        m_data.clear();

        m_firstFreeSlot = NoFreeSlot;
    }

    template<typename DataType, typename IndexType>
    inline IndexType IndexedDataVector<DataType, IndexType>::GetFreeSlotIndex()
    {
        IndexType freeSlotIndex = static_cast<IndexType>(m_indices.size());

        if (freeSlotIndex == NoFreeSlot)
        {
            // the vector is full
            return NoFreeSlot;
        }

        if (m_firstFreeSlot == NoFreeSlot)
        {
            // If there's no free slot, add on to the end.
            m_indices.push_back(freeSlotIndex);
        }
        else
        {
            // Fill the free slot. m_indices uses it's empty slots to store a linked list (via indices) to other empty slots.
            freeSlotIndex = m_firstFreeSlot;
            m_firstFreeSlot = m_indices.at(m_firstFreeSlot);
            m_indices.at(freeSlotIndex) = static_cast<IndexType>(m_data.size());
        }

        // The data itself is always packed and m_indices points at it, so push a new entry to the back.
        m_data.push_back(DataType());
        m_dataToIndices.push_back(freeSlotIndex);

        return freeSlotIndex;
    }

    template<typename DataType, typename IndexType>
    inline void IndexedDataVector<DataType, IndexType>::RemoveIndex(IndexType index)
    {
        IndexType dataIndex = m_indices.at(index);

        // Copy the back light on top of this one.
        m_data.at(dataIndex) = m_data.back();
        m_dataToIndices.at(dataIndex) = m_dataToIndices.back();

        // Update the index of the moved light
        m_indices.at(m_dataToIndices.at(dataIndex)) = dataIndex;

        // Pop the back
        m_data.pop_back();
        m_dataToIndices.pop_back();

        // Use free slot to link to next free slot
        m_indices.at(index) = m_firstFreeSlot;
        m_firstFreeSlot = index;
    }
    
    template<typename DataType, typename IndexType>
    inline void IndexedDataVector<DataType, IndexType>::RemoveData(DataType* data)
    {
        IndexType indexForData = GetIndexForData(data);
        if (indexForData != NoFreeSlot)
        {
            RemoveIndex(indexForData);
        }
    }

    template<typename DataType, typename IndexType>
    inline DataType& IndexedDataVector<DataType, IndexType>::GetData(IndexType index)
    {
        return m_data.at(m_indices.at(index));
    }

    template<typename DataType, typename IndexType>
    inline const DataType& IndexedDataVector<DataType, IndexType>::GetData(IndexType index) const
    {
        return m_data.at(m_indices.at(index));
    }

    template<typename DataType, typename IndexType>
    inline size_t IndexedDataVector<DataType, IndexType>::GetDataCount() const
    {
        return m_data.size();
    }

    template<typename DataType, typename IndexType>
    inline AZStd::vector<DataType>& IndexedDataVector<DataType, IndexType>::GetDataVector()
    {
        return m_data;
    }

    template<typename DataType, typename IndexType>
    inline const AZStd::vector<DataType>& IndexedDataVector<DataType, IndexType>::GetDataVector() const
    {
        return m_data;
    }

    template<typename DataType, typename IndexType>
    inline const AZStd::vector<IndexType>& IndexedDataVector<DataType, IndexType>::GetDataToIndexVector() const
    {
        return m_dataToIndices;
    }

    template<typename DataType, typename IndexType>
    IndexType IndexedDataVector<DataType, IndexType>::GetRawIndex(IndexType index) const
    {
        return m_indices.at(index);
    }
    
    template<typename DataType, typename IndexType>
    IndexType IndexedDataVector<DataType, IndexType>::GetIndexForData(const DataType* data) const
    {
        if (data >= &m_data.front() && data <= &m_data.back())
        {
            return m_dataToIndices.at(data - &m_data.front());
        }
        return NoFreeSlot;
    }
} // namespace AZ::Render
