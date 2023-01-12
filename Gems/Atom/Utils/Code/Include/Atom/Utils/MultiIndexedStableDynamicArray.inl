/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/createdestroy.h>

namespace AZ
{

    // MultiIndexedStableDynamicArray

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::MultiIndexedStableDynamicArray(allocator_type allocator)
        : m_allocator(allocator)
    {}

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::~MultiIndexedStableDynamicArray()
    {
        // Deallocate the pages and check for allocated items since that may mean there are
        // outstanding handles that we should warn the user about.

        size_t occupiedPageCount = 0;
        size_t orphanedItemCount = 0;

        Page* page = m_firstPage;
        while (page)
        {
            if (!page->IsEmpty())
            {
                ++occupiedPageCount;
                orphanedItemCount += page->GetItemCount();
            }
            Page* pageToDelete = page;
            page = page->m_nextPage;
            pageToDelete->~Page();
            m_allocator.deallocate(pageToDelete, sizeof(Page), AZStd::alignment_of<Page>::value);
        }

        AZ_Warning("MultiIndexedStableDynamicArray", occupiedPageCount == 0,
            "MultiIndexedStableDynamicArray is being deleted but there are still %zu outstanding handles on %zu pages. Handles that "
            "are not freed before MultiIndexedStableDynamicArray is removed will point to garbage memory.",
            orphanedItemCount, occupiedPageCount
        );
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::insert(const value_types&... values) -> Handle
    {
        return emplace(values...);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::insert(value_types&&... values) -> Handle
    {
        return emplace(AZStd::move(values...));
    }

    template<size_t RowIndex, typename... value_types, typename PageType, typename TupleType>
    static void ConstructElement(
        PageType* page,
        MultiIndexedStableDynamicArrayPageIndexType pageElementIndex,
        TupleType& valuesTuple)
    {
        using DataType = AZStd::tuple_element_t<RowIndex, AZStd::tuple<value_types...>>;
        //DataType& item = page->GetItem<RowIndex>(pageElementIndex);
        //item = AZStd::get<RowIndex>(valuesTuple);
        DataType* item = page->GetItem<RowIndex>(pageElementIndex);
        // must be copy constructable
        AZStd::Internal::construct<DataType*>::single(item, AZStd::get<RowIndex>(valuesTuple));
    }

    
    template<typename... value_types, typename PageType, typename ArgumentTuple, size_t... Ints>
    static void ConstructElementsInner(
        PageType* page,
        MultiIndexedStableDynamicArrayPageIndexType pageElementIndex,
        ArgumentTuple& argumentTuple,
        AZStd::index_sequence<Ints...>)
    {
        (ConstructElement<Ints, value_types...>(page, pageElementIndex, argumentTuple), ...);
    }

    template<size_t RowCount, typename...value_types, typename PageType, typename ArgumentTuple>
    static void ConstructElements(
        PageType* page, MultiIndexedStableDynamicArrayPageIndexType pageElementIndex, ArgumentTuple& argumentTuple
        )
    {
        constexpr auto tupleIndices = AZStd::make_index_sequence<RowCount>{};
        ConstructElementsInner<value_types...>(page, pageElementIndex, argumentTuple, tupleIndices);
        //using DataType = AZStd::tuple_element_t<RowIndices, AZStd::tuple<value_types...>>;
        //AZStd::tuple<value_types&...> itemReferences;
        //(void*)indexSequence;
        //((AZStd::get<RowIndices>(itemReferences) = page->GetItem<RowIndices>(pageElementIndex)), ...);
        //((AZStd::get<RowIndices>(itemReferences) = AZStd::get<RowIndices>(AZStd::forward<AZStd::tuple<value_types...>>(valuesTuple))), ...);
    }

    template<typename TupleType, size_t RowIndex, typename PageType>
    static void DestructElement(PageType* page, MultiIndexedStableDynamicArrayPageIndexType pageElementIndex)
    {
        using DataType = AZStd::tuple_element_t<RowIndex, TupleType>;
        //DataType& item = page->GetItem<RowIndex>(pageElementIndex);
        //item.~DataType();
        DataType* item = page->GetItem<RowIndex>(pageElementIndex);
        item->~DataType();
    }

    template<typename TupleType, typename PageType, size_t... Ints>
    static void DestructElementsInner(
        PageType* page,
        MultiIndexedStableDynamicArrayPageIndexType pageElementIndex,
        AZStd::index_sequence<Ints...>)
    {
        (DestructElement<TupleType, Ints>(page, pageElementIndex), ...);
    }

    template<typename TupleType, size_t RowCount, typename PageType>
    static void DestructElements(PageType* page, MultiIndexedStableDynamicArrayPageIndexType pageElementIndex)
    {
        constexpr auto tupleIndices = AZStd::make_index_sequence<RowCount>{};
        DestructElementsInner<TupleType>(page, pageElementIndex, tupleIndices);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<class... Args>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::emplace(Args&&... args)
        -> Handle
    { 
        using ArgumentTuple = AZStd::tuple<Args...>;
        ArgumentTuple argumentTuple{ args... };
        // Try to find a page we can fit this in.
        while (m_firstAvailablePage)
        {
            MultiIndexedStableDynamicArrayPageIndexType pageElementIndex = m_firstAvailablePage->Reserve();
            if (pageElementIndex != MultiIndexedStableDynamicArrayInvalidPageIndex)
            {                    
                ConstructElements<AZStd::tuple_size<ArgumentTuple>::value, value_types...>(
                    m_firstAvailablePage, pageElementIndex, argumentTuple);

                ++m_itemCount;
                return Handle(m_firstAvailablePage, pageElementIndex);
            }
            if (!m_firstAvailablePage->m_nextPage)
            {
                // no more pages, break and make a new one.
                break;
            }
            m_firstAvailablePage = m_firstAvailablePage->m_nextPage;
        }

        // No page to emplace in, so make a new page
        Page* page = AddPage();
        if (m_firstAvailablePage)
        {
            m_firstAvailablePage->m_nextPage = page;
        }
        else
        {
            // If m_firstAvailablePage was nullptr, then there were no pages so m_firstPage would also be null, and needs to be set to the new page.
            m_firstPage = page;
        }

        // A new page was created since there was no room in any other page, so this new page will also be the first page where slots are available.
        m_firstAvailablePage = page;
        
        MultiIndexedStableDynamicArrayPageIndexType pageElementIndex = m_firstAvailablePage->Reserve();

        ConstructElements<AZStd::tuple_size<ArgumentTuple>::value, value_types...>(m_firstAvailablePage, pageElementIndex, argumentTuple);

        ++m_itemCount;
        return Handle(m_firstAvailablePage, pageElementIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::erase(Handle& handle)
    {
        if (!handle.IsValid())
        {
            return;
        }

        // Update the first free page if the page this item is being removed from is earlier in the list.
        Page* page = reinterpret_cast<Page*>(handle.m_page);
        if (page->m_pageIndex < m_firstAvailablePage->m_pageIndex)
        {
            m_firstAvailablePage = page;
        }

        // Destroy the data in the handle, invalidate the handle, and free the spot that it points to.
        page->Free(handle.m_index);
        handle.Invalidate();
        --m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    size_t MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::size() const
    {
        return m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::GetParallelRanges()
        -> AZStd::vector<AZStd::pair<pageIterator, pageIterator>>
    {
        AZStd::vector<AZStd::pair<pageIterator, pageIterator>> pageIterators;
        Page* page = m_firstPage;
        while (page)
        {
            if (!page->IsEmpty())
            {
                pageIterators.push_back({ pageIterator(page), pageIterator(nullptr) });
            }
            page = page->m_nextPage;
        }
        return pageIterators;
    }

    // TODO: Not sure we can support this if we aren't treating handles as unique_ptrs, unless we add an indirection table
    /*
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::DefragmentHandle(Handle& handle)
    {
        if (!handle.IsValid() || reinterpret_cast<Page*>(handle.m_page)->IsFull())
        {
            // If this handle has memory in a full page, it's already compact.
            return;
        }

        // Try to find a page we can fit this in.
        while (m_firstAvailablePage)
        {
            // if the first page with space available is the page this item is already in, there's not a better page to be in so let it be.
            if (m_firstAvailablePage == handle.m_page)
            {
                break;
            }

            size_t pageItemIndex = m_firstAvailablePage->Reserve();
            if (pageItemIndex != Page::InvalidPage)
            {
                // Found a better page, move the data to it.
                *m_firstAvailablePage->GetItem(pageItemIndex) = AZStd::move(*handle);
                reinterpret_cast<Page*>(handle.m_page)->Free(handle.m_data);
                handle.m_data = m_firstAvailablePage->GetItem(pageItemIndex);
                handle.m_page = m_firstAvailablePage;
                break;
            }
            m_firstAvailablePage = m_firstAvailablePage->m_nextPage;
        }

    }
    */

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::ReleaseEmptyPages()
    {
        Page* page = m_firstPage;
        Page** previousNextPagePointer = &m_firstPage;

        while (page)
        {
            if (page->IsEmpty())
            {
                *previousNextPagePointer = page->m_nextPage;
                Page* pageToDellocate = page;
                page = page->m_nextPage;
                m_allocator.deallocate(pageToDellocate, sizeof(Page), AZStd::alignment_of<Page>::value);
            }
            else
            {
                previousNextPagePointer = &page->m_nextPage;
                page = page->m_nextPage;
            }
        }

        // Start by assuming the first available page is the first page (if there are no pages then both will be nullptr)
        m_firstAvailablePage = m_firstPage;

        // If there are any pages at all, then recalculate the first available page.
        if (m_firstAvailablePage)
        {
            // If all pages are full this will cause m_firstAvailablePage to point to the last page, otherwise it will be a page with space in it.
            while (m_firstAvailablePage->m_nextPage && m_firstAvailablePage->IsFull())
            {
                m_firstAvailablePage = m_firstAvailablePage->m_nextPage;
            }
        }
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayMetrics MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::GetMetrics()
    {
        MultiIndexedStableDynamicArrayMetrics metrics;
        Page* page = m_firstPage;

        while (page)
        {
            size_t itemCount = page->GetItemCount();
            metrics.m_totalElements += itemCount;
            metrics.m_elementsPerPage.push_back(itemCount);
            if (itemCount == 0)
            {
                ++metrics.m_emptyPages;
            }
            page = page->m_nextPage;
        }

        size_t pageCount = metrics.m_elementsPerPage.size();
        size_t pagesWithItems = pageCount - metrics.m_emptyPages;

        // This calculates a number between 0 and 1 that represents how densely the pages are packed. If this number starts to get close
        // to 0, that means the items are very sparsely packed and it may be worth calling DefragmentHandle() on the handles to repack them
        // to reduce memory consumption and increase iteration time.
        if (pagesWithItems > 0)
        {
            float elementDensity = float(metrics.m_totalElements) / float(ElementsPerPage);
            metrics.m_itemToPageRatio = ceil(elementDensity) / pagesWithItems;
        }
        else
        {
            metrics.m_itemToPageRatio = 0.0f;
        }

        return metrics;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::begin() -> iterator
    {
        return iterator(m_firstPage);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::cbegin() const -> const_iterator
    {
        return const_iterator(m_firstPage);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::end() -> iterator
    {
        return iterator();
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::cend() const -> const_iterator
    {
        return const_iterator();
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::AddPage() -> Page*
    {
        void* pageMemory = m_allocator.allocate(sizeof(Page), AZStd::alignment_of<Page>::value);
        Page* page = new (pageMemory) Page();
        page->m_pageIndex = ++m_pageCounter;
        page->m_container = this;
        return page;
    }


    // MultiIndexedStableDynamicArray::Page


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::Page()
    {
        m_bits.fill(0);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayPageIndexType MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::Reserve()
    {
        for (; m_bitStartIndex < NumUint64_t; ++m_bitStartIndex)
        {
            if (m_bits[m_bitStartIndex] != FullBits)
            {
                // Find the free slot, mark it, and return the index.
                uint64_t freeSlot = az_ctz_u64(~m_bits[m_bitStartIndex]);
                m_bits[m_bitStartIndex] |= 1ull << freeSlot;
                ++m_itemCount;

                return static_cast<MultiIndexedStableDynamicArrayPageIndexType>(freeSlot + 64 * m_bitStartIndex);
            }
        }
        return MultiIndexedStableDynamicArrayInvalidPageIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::Free(
        MultiIndexedStableDynamicArrayPageIndexType index)
    {
        using TupleType = AZStd::tuple<value_types...>;
        DestructElements<TupleType, AZStd::tuple_size<TupleType>::value>(this, index);

        // This item's flag will be in the uint64_t at index >> 6 (index / 64). Mark the appropriate bit as 0 (Free).
        AZ_Assert(m_bits[index >> 6] & (1ull << (index & 0x3F)), "Freeing item that is already marked as free!"); // The IsValid() check on handles should prevent this ever happening.
        m_bits[index >> 6] &= ~(1ull << (index & 0x3F));
        // Set the bit start index so the next Reserve() starts on a uint_64 that likely has space.
        m_bitStartIndex = AZStd::min(static_cast<size_t>(index) >> 6, m_bitStartIndex);

        --m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::IsFull() const
    {
        return m_itemCount == ElementsPerPage;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::IsEmpty() const
    {
        return m_itemCount == 0;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    AZStd::tuple_element_t<RowIndex, AZStd::tuple<value_types...>>* MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::GetItem(
        MultiIndexedStableDynamicArrayPageIndexType index)
    {
        //using AlignedStorageType = AZStd::tuple_element_t<RowIndex,AZStd::tuple<AZStd::aligned_storage_t<ElementsPerPage * sizeof(value_types), alignof(value_types)>...>>;
        using DataType = AZStd::tuple_element_t<RowIndex, AZStd::tuple<value_types...>>;
        return (reinterpret_cast<DataType*>(&AZStd::get<RowIndex>(m_data)) + index);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    size_t MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::Page::GetItemCount() const
    {
        return m_itemCount;
    }


    // MultiIndexedStableDynamicArray::iterator


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::iterator(Page* firstPage)
        : m_page(firstPage)
    {
        // SkipEmptyPages() will move the iterator past any empty pages at the beginning of the list of pages
        // and return false if it runs out of pages and they're all empty. If this happens, then don't alter
        // anything in the iterator so it's equivalent to the .end() iterator.
        if (SkipEmptyPages())
        {
            // Setup the bit group from the first page with items in it
            m_remainingBitsInBitGroup = m_page->m_bits.at(m_bitGroupIndex);

            // Set up the item pointer and increments the bits.
            AdvanceIterator();
        }
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>
        MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::operator*() const
    {
        return MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>(m_page, m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto* MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::GetItem() const
    {
        return m_page->GetItem<RowIndex>(m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::operator==(const this_type& rhs) const
    {
        return rhs.m_itemIndex == m_itemIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::operator++() -> this_type&
    {
        // If this bit group is finished, find the next bit group with bits in it.
        if (m_remainingBitsInBitGroup == 0)
        {
            // skip the next bit group in the page until one is found with entries
            for (++m_bitGroupIndex; m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0; ++m_bitGroupIndex)
            {
            }

            if (m_bitGroupIndex == Page::NumUint64_t)
            {
                // Done with this page, on to the next.
                m_bitGroupIndex = 0;
                m_page = m_page->m_nextPage;

                // skip empty pages
                if (!SkipEmptyPages())
                {
                    // If SkipEmptyPages() returns false, it means it reached the last page without finding anything. At this point the iterator is in its end state, so just return;
                    return *this;
                }
            }

            m_remainingBitsInBitGroup = m_page->m_bits.at(m_bitGroupIndex);
        }

        // Set up the item pointer and increments the bits.
        AdvanceIterator();

        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::SkipEmptyPages()
    {
        // skip all initial empty pages.
        while (m_page && m_page->IsEmpty())
        {
            m_page = m_page->m_nextPage;
        }

        // If the page is null, it's at the end. This sets m_item to nullptr so that it == MultiIndexedStableDynamicArray::End().
        if (m_page == nullptr)
        {
            m_itemIndex = MultiIndexedStableDynamicArrayInvalidPageIndex;
            return false;
        }

        // skip the empty bitfields in the page
        for (; m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0; ++m_bitGroupIndex)
        {
        }

        return true;
    }


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::iterator::AdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_itemIndex = static_cast<MultiIndexedStableDynamicArrayPageIndexType>(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // MultiIndexedStableDynamicArray::const_iterator

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::const_iterator::const_iterator(Page* firstPage)
        : base_type(firstPage)
    {
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto* MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::const_iterator::GetItem() const
    {
        return base_type::m_page->GetItem<RowIndex>(base_type::m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::const_iterator::operator++() -> this_type&
    {
        base_type::operator++();
        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::const_iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    // MultiIndexedStableDynamicArray::pageIterator

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::pageIterator(Page* page)
        : m_page(page)
    {
        if (m_page != nullptr)
        {
            // SkipEmptyBitGroups() will skip all the initial empty bit groups that may exist in the page.
            SkipEmptyBitGroups();
        }
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...> MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::
        pageIterator::operator*() const
    {
        return MultiIndexedStableDynamicArrayHandle(m_page, m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto* MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::GetItem() const
    {
        return m_page->GetItem<RowIndex>(m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::operator==(const this_type& rhs) const
    {
        return rhs.m_itemIndex == m_itemIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::operator++() -> this_type&
    {
        // If this bit group is finished, find the next bit group with bits in it.
        if (m_remainingBitsInBitGroup == 0)
        {
            ++m_bitGroupIndex;

            // skip the next bit group in the page until one is found with entries
            SkipEmptyBitGroups();
        }
        else
        {
            // Set up the item pointer and increments the bits.
            SetItemAndAdvanceIterator();
        }

        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::SkipEmptyBitGroups()
    {
        // skip the next bit group in the page until one is found with entries
        while (m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0)
        {
            ++m_bitGroupIndex;
        }

        if (m_bitGroupIndex >= Page::NumUint64_t)
        {
            // Done with this page, so it's at the end of the page iterator.
            m_itemIndex = MultiIndexedStableDynamicArrayInvalidPageIndex;
            return;
        }

        // Set up the bit group from the index found earlier.
        m_remainingBitsInBitGroup = m_page->m_bits.at(m_bitGroupIndex);

        // Set up the item pointer and increments the bits.
        SetItemAndAdvanceIterator();

    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArray<ElementsPerPage, Allocator, value_types...>::pageIterator::SetItemAndAdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_itemIndex = static_cast<MultiIndexedStableDynamicArrayPageIndexType>(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // MultiIndexedStableDynamicArray::Handle

    
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::MultiIndexedStableDynamicArrayHandle(
        PageType* page, MultiIndexedStableDynamicArrayPageIndexType index)
        : m_page(page)
        , m_index(index)
    {
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::MultiIndexedStableDynamicArrayHandle(
        MultiIndexedStableDynamicArrayHandle&& other)
    {
        *this = AZStd::move(other);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::~MultiIndexedStableDynamicArrayHandle()
    {
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::operator=(
        MultiIndexedStableDynamicArrayHandle&& other)
        -> MultiIndexedStableDynamicArrayHandle&
    {
        if (this != static_cast<void*>(&other))
        {
            Free();
            m_index = other.m_index;
            m_page = other.m_page;
            other.Invalidate();
        }
        return *this;
    }
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::Free()
    {
        if (IsValid())
        {
            m_page->m_container->erase(*this);
        }
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::IsValid() const
    {
        return m_index != MultiIndexedStableDynamicArrayInvalidPageIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::IsNull() const
    {
        return m_index == MultiIndexedStableDynamicArrayInvalidPageIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::Invalidate()
    {
        m_index = MultiIndexedStableDynamicArrayInvalidPageIndex;
        m_page = nullptr;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto* MultiIndexedStableDynamicArrayHandle<ElementsPerPage, Allocator, value_types...>::GetItem() const
    {
        return m_page->GetItem<RowIndex>(m_index);
    }

} // end namespace AZ
