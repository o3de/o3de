/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ::Render
{
    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline uint32_t PagedDataVector<Key, DataType, ElementsPerPage>::Add()
    {
        // The page to use
        uint16_t pageIndexResult = Invalid16BitIndex;
        // The element within the page
        uint16_t elementIndexResult = Invalid16BitIndex;
        // The combined index that encodes both the page and the element
        uint32_t dataIndex = Invalid32BitIndex;

        // Iterate over the free list for each page, and find the first available index
        for (size_t pageIndex = 0; pageIndex < m_indexFreeLists.size(); ++pageIndex)
        {
            if (!m_indexFreeLists[pageIndex].empty())
            {
                // We have found a free index, use it
                pageIndexResult = static_cast<uint16_t>(pageIndex);
                elementIndexResult = m_indexFreeLists[pageIndexResult].back();
                m_indexFreeLists[pageIndexResult].pop_back();
                dataIndex = EncodeIndex(pageIndexResult, elementIndexResult);
                break;
            }
        }

        // If there are no free list entries, create a new page
        if (pageIndexResult == Invalid16BitIndex)
        {
            m_data.push_back(new DataPage{});
            m_indexFreeLists.push_back(CreateFreeList());

            // Now we have a free index, use it
            pageIndexResult = static_cast<uint16_t>(m_data.size() - 1);
            elementIndexResult = m_indexFreeLists[pageIndexResult].back();
            m_indexFreeLists[pageIndexResult].pop_back();
            dataIndex = EncodeIndex(pageIndexResult, elementIndexResult);
        }
        m_itemCount++;

        return dataIndex;
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline void PagedDataVector<Key, DataType, ElementsPerPage>::Remove(uint32_t dataIndex)
    {
        PageElementIndex decodedIndex = DecodeIndex(dataIndex);

        // Clear the entry with one that is default constructed
        (*m_data[decodedIndex.m_pageIndex])[decodedIndex.m_elementIndex] = DataType{};

        // Add the entry to the freelist
        m_indexFreeLists[decodedIndex.m_pageIndex].push_back(decodedIndex.m_elementIndex);

        AZ_Assert(m_itemCount > 0, "Attempting to remove an item from an empty container.");
        m_itemCount--;
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline DataType& PagedDataVector<Key, DataType, ElementsPerPage>::operator[](uint32_t index)
    {
        PageElementIndex decodedIndex = DecodeIndex(index);
        return (*m_data[decodedIndex.m_pageIndex])[decodedIndex.m_elementIndex];
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline const DataType& PagedDataVector<Key, DataType, ElementsPerPage>::operator[](uint32_t index) const
    {
        PageElementIndex decodedIndex = DecodeIndex(index);
        return (*m_data[decodedIndex.m_pageIndex])[decodedIndex.m_elementIndex];
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline void PagedDataVector<Key, DataType, ElementsPerPage>::Reset()
    {
        m_data.clear();
        m_indexFreeLists.clear();
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline uint32_t PagedDataVector<Key, DataType, ElementsPerPage>::EncodeIndex(uint16_t pageIndex, uint16_t elementIndex) const
    {
        return static_cast<uint32_t>(pageIndex) << 16 | static_cast<uint32_t>(elementIndex);
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    inline PageElementIndex PagedDataVector<Key, DataType, ElementsPerPage>::DecodeIndex(uint32_t encodedIndex) const
    {
        uint16_t elementIndex = encodedIndex & 0xFFFF;
        uint16_t pageIndex = encodedIndex >> 16;
        return PageElementIndex{ pageIndex, elementIndex };
    }

    template<class Key, class DataType, uint16_t ElementsPerPage>
    constexpr typename PagedDataVector<Key, DataType, ElementsPerPage>::PageFreeList PagedDataVector<
        Key,
        DataType,
        ElementsPerPage>::
        CreateFreeList() const
    {
        PageFreeList freeList{};
        // We pop from the back of the freeList, so populate it with decreasing numbers to fill the data pages from front to back
        for (uint16_t i = 0; i < ElementsPerPage; ++i)
        {
            freeList.push_back(ElementsPerPage - i - 1);
        }
        return freeList;
    }
} // namespace AZ::Render
