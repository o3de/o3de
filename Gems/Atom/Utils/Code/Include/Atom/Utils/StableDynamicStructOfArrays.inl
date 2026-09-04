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

    // StableDynamicStructOfArrays

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::StableDynamicStructOfArrays(allocator_type allocator)
        : m_allocator(allocator)
    {}

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::~StableDynamicStructOfArrays()
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

        AZ_Warning("StableDynamicStructOfArrays", occupiedPageCount == 0,
            "StableDynamicStructOfArrays is being deleted but there are still %zu outstanding handles on %zu pages. Handles that "
            "are not freed before StableDynamicStructOfArrays is removed will point to garbage memory.",
            orphanedItemCount, occupiedPageCount
        );
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::insert(const value_types&... values) -> Handle
    {
        return emplace(AZStd::forward_as_tuple(values)...);
    }

    template<class ValueType, class... Args>
    static auto EmplaceElement(ValueType elementPtr, Args&&... elementArgs)
    {
        AZStd::construct_at(elementPtr, AZStd::forward<Args>(elementArgs)...);
    }

    template<class ValueType, class... Args>
    static auto EmplaceEachElement(ValueType* elementPtr, AZStd::tuple<Args...>& elementArgs)
    {
        auto ConstructElement = [elementPtr](auto&&... args)
        {
            EmplaceElement(elementPtr, AZStd::forward<decltype(args)>(args)...);
        };
        AZStd::apply(ConstructElement, elementArgs);
    }

    template<class... value_types, size_t... Indices, class... TupleArgs>
    static auto EmplaceTupleElementsHelper(
        AZStd::tuple<value_types*...>& elements, AZStd::index_sequence<Indices...>, TupleArgs&&... tupleArgs)
    {
        (EmplaceEachElement(AZStd::get<Indices>(elements), tupleArgs), ...);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<class... TupleArgs>
    typename StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Handle
        StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::EmplaceTupleElements(
            Page* page, size_t pageElementIndex, TupleArgs&&... tupleArgs)
    {
        ItemTupleType items = page->GetItems(pageElementIndex);
        EmplaceTupleElementsHelper(items, AZStd::make_index_sequence<sizeof...(TupleArgs)>{}, AZStd::forward<TupleArgs>(tupleArgs)...);
        ++m_itemCount;
        return Handle(items, page);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<typename... TupleArgs>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::emplace(TupleArgs&&... args)
        -> Handle
    { 
        // Try to find a page we can fit this in.
        while (m_firstAvailablePage)
        {
            size_t pageElementIndex = m_firstAvailablePage->Reserve();
            if (pageElementIndex != Page::InvalidPage)
            {
                return EmplaceTupleElements(m_firstAvailablePage, pageElementIndex, AZStd::forward<TupleArgs>(args)...);
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
        
        size_t pageElementIndex = m_firstAvailablePage->Reserve();

        return EmplaceTupleElements(m_firstAvailablePage, pageElementIndex, AZStd::forward<TupleArgs>(args)...);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::erase(Handle& handle)
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
        AZStd::apply(
            [](value_types *... data)
            {
                ((AZStd::destroy_at(data)), ...);
            },
            handle.m_data);

        page->Free(handle.m_data);
        handle.Invalidate();
        --m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    size_t StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::size() const
    {
        return m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::GetParallelRanges()
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

    template<typename ItemTupleType, size_t... Ints>
    static void MoveData(
        ItemTupleType& sourceItem,
        ItemTupleType& destinationItem,
        AZStd::index_sequence<Ints...>)
    {
        ((AZStd::get<Ints>(destinationItem) = AZStd::move(AZStd::get<Ints>(sourceItem))), ...);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::DefragmentHandle(Handle& handle)
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
                // Found a better page, move the data from the handle to the new page.
                ItemTupleType destinationItems = m_firstAvailablePage->GetItems(pageItemIndex);
                MoveData(*handle, destinationItems, AZStd::make_index_sequence<AZStd::tuple_size_v<ItemTupleType>>{});
                reinterpret_cast<Page*>(handle.m_page)->Free(handle.m_data);
                handle.m_data = m_firstAvailablePage->GetItems(pageItemIndex);
                handle.m_page = m_firstAvailablePage;
                break;
            }
            m_firstAvailablePage = m_firstAvailablePage->m_nextPage;
        }

    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::ReleaseEmptyPages()
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
    StableDynamicStructOfArraysMetrics StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::GetMetrics()
    {
        StableDynamicStructOfArraysMetrics metrics;
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
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::begin() -> iterator
    {
        return iterator(m_firstPage);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::cbegin() const -> const_iterator
    {
        return const_iterator(m_firstPage);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::end() -> iterator
    {
        return iterator();
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::cend() const -> const_iterator
    {
        return const_iterator();
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::AddPage() -> Page*
    {
        void* pageMemory = m_allocator.allocate(sizeof(Page), AZStd::alignment_of<Page>::value);
        Page* page = new (pageMemory) Page();
        page->m_pageIndex = ++m_pageCounter;
        page->m_container = this;
        return page;
    }

    // StableDynamicStructOfArrays::Page


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::Page()
    {
        m_bits.fill(0);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    size_t StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::Reserve()
    {
        for (; m_bitStartIndex < NumUint64_t; ++m_bitStartIndex)
        {
            if (m_bits[m_bitStartIndex] != FullBits)
            {
                // Find the free slot, mark it, and return the index.
                uint64_t freeSlot = az_ctz_u64(~m_bits[m_bitStartIndex]);
                m_bits[m_bitStartIndex] |= 1ull << freeSlot;
                ++m_itemCount;

                return  static_cast<size_t>(freeSlot + 64 * m_bitStartIndex);
            }
        }
        return InvalidPage;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::Free(ItemTupleType item)
    {
        // Use the first row of data to get the index within the page
        AZStd::tuple_element_t<0, ItemTupleType> pageStart = reinterpret_cast<AZStd::tuple_element_t<0, ItemTupleType>>(&AZStd::get<0>(m_data));
        AZStd::tuple_element_t<0, ItemTupleType> itemStart = AZStd::get<0>(item);
        size_t index =  itemStart - pageStart;

        // This item's flag will be in the uint64_t at index >> 6 (index / 64). Mark the appropriate bit as 0 (Free).
        AZ_Assert(m_bits[index >> 6] & (1ull << (index & 0x3F)), "Freeing item that is already marked as free!"); // The IsValid() check on handles should prevent this ever happening.
        m_bits[index >> 6] &= ~(1ull << (index & 0x3F));
        // Set the bit start index so the next Reserve() starts on a uint_64 that likely has space.
        m_bitStartIndex = AZStd::min(static_cast<size_t>(index) >> 6, m_bitStartIndex);

        --m_itemCount;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::IsFull() const
    {
        return m_itemCount == ElementsPerPage;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::IsEmpty() const
    {
        return m_itemCount == 0;
    }


    template<typename PageType, typename ItemTupleType, size_t... Ints>
    static void SetDataPtrsOnItem(
        ItemTupleType& items,
        PageType page,
        size_t pageElementIndex,
        AZStd::index_sequence<Ints...>)
    {
        ((AZStd::get<Ints>(items) = page->GetItem<Ints>(pageElementIndex)), ...);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    typename StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::ItemTupleType
        StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::GetItems(size_t elementIndex)
    {
        ItemTupleType items;

        // Both m_data and items need to be expanded here, so instead of using AZStd::apply, we generate indices for the tuple
        // and use them to both expand the tuple and access the correct row of the page's m_data via GetItem<>
        constexpr auto tupleIndices = AZStd::make_index_sequence<AZStd::tuple_size_v<ItemTupleType>>{};
        SetDataPtrsOnItem(items, this, elementIndex, tupleIndices);

        return items;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto* StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::GetItem(
        size_t index)
    {
        using DataType = AZStd::tuple_element_t<RowIndex, AZStd::tuple<value_types...>>;
        return (reinterpret_cast<DataType*>(&AZStd::get<RowIndex>(m_data)) + index);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    size_t StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page::GetItemCount() const
    {
        return m_itemCount;
    }


    // StableDynamicStructOfArrays::iterator


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::iterator(Page* firstPage)
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
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::operator*() const -> const this_type&
    {
        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto& StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::GetItem() const
    {
        return *m_page->GetItem<RowIndex>(m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::operator==(const this_type& rhs) const
    {
        return rhs.m_itemIndex == m_itemIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::operator++() -> this_type&
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
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::SkipEmptyPages()
    {
        // skip all initial empty pages.
        while (m_page && m_page->IsEmpty())
        {
            m_page = m_page->m_nextPage;
        }

        // If the page is null, it's at the end. This sets m_item to nullptr so that it == StableDynamicStructOfArrays::End().
        if (m_page == nullptr)
        {
            m_itemIndex = Page::InvalidPage;
            return false;
        }

        // skip the empty bitfields in the page
        for (; m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0; ++m_bitGroupIndex)
        {
        }

        return true;
    }


    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator::AdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_itemIndex = static_cast<size_t>(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // StableDynamicStructOfArrays::const_iterator

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::const_iterator::const_iterator(Page* firstPage)
        : base_type(firstPage)
    {
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto& StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::const_iterator::GetItem() const
    {
        return *base_type::m_page->GetItem<RowIndex>(base_type::m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::const_iterator::operator++() -> this_type&
    {
        base_type::operator++();
        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::const_iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    // StableDynamicStructOfArrays::pageIterator

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::pageIterator(Page* page)
        : m_page(page)
    {
        if (m_page != nullptr)
        {
            // SkipEmptyBitGroups() will skip all the initial empty bit groups that may exist in the page.
            SkipEmptyBitGroups();
        }
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::operator*() const -> const this_type&
    {
        return *this;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    template<size_t RowIndex>
    auto& StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::GetItem() const
    {
        return *m_page->GetItem<RowIndex>(m_itemIndex);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::operator==(const this_type& rhs) const
    {
        return rhs.m_itemIndex == m_itemIndex;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    bool StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::operator++() -> this_type&
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
    auto StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::SkipEmptyBitGroups()
    {
        // skip the next bit group in the page until one is found with entries
        while (m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0)
        {
            ++m_bitGroupIndex;
        }

        if (m_bitGroupIndex >= Page::NumUint64_t)
        {
            // Done with this page, so it's at the end of the page iterator.
            m_itemIndex = Page::InvalidPage;
            return;
        }

        // Set up the bit group from the index found earlier.
        m_remainingBitsInBitGroup = m_page->m_bits.at(m_bitGroupIndex);

        // Set up the item pointer and increments the bits.
        SetItemAndAdvanceIterator();

    }

    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    void StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator::SetItemAndAdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_itemIndex = static_cast<size_t>(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // StableDynamicStructOfArrays::WeakHandle
    template<typename... value_types>
    StableDynamicStructOfArraysWeakHandle<value_types...>::StableDynamicStructOfArraysWeakHandle(const ItemTupleType& data)
        : m_data(data)
    {
    }

    template<typename... value_types>
    bool StableDynamicStructOfArraysWeakHandle<value_types...>::IsValid() const
    {
        return AZStd::get<0>(m_data) != nullptr;
    }

    template<typename... value_types>
    bool StableDynamicStructOfArraysWeakHandle<value_types...>::IsNull() const
    {
        return AZStd::get<0>(m_data) == nullptr;
    }

    template<typename... value_types>
    inline typename StableDynamicStructOfArraysWeakHandle<value_types...>::ItemTupleType&
        StableDynamicStructOfArraysWeakHandle<value_types...>::operator*()
    {
        return m_data;
    }

    template<typename... value_types>
    template<size_t RowIndex>
    inline auto& StableDynamicStructOfArraysWeakHandle<value_types...>::GetItem() const
    {
        return *AZStd::get<RowIndex>(m_data);
    }

    // StableDynamicStructOfArrays::Handle


    template<typename... value_types>
    template<typename PageType>
    inline StableDynamicStructOfArraysHandle<value_types...>::StableDynamicStructOfArraysHandle(
        ItemTupleType data, PageType* page)
        : m_data(data)
        , m_page(page)
    {
        // Store container type information in the non-capturing lambda callback so the Handle itself doesn't need it.
        m_destructorCallback = [](void* typelessHandlePointer)
        {
            StableDynamicStructOfArraysHandle* handle = static_cast<StableDynamicStructOfArraysHandle*>(typelessHandlePointer);
            static_cast<PageType*>(handle->m_page)->m_container->erase(*handle);
        };        
    }

    template<typename... value_types>
    inline StableDynamicStructOfArraysHandle<value_types...>::StableDynamicStructOfArraysHandle(
        StableDynamicStructOfArraysHandle&& other)
    {
        *this = AZStd::move(other);
    }

    template<typename... value_types>
    inline StableDynamicStructOfArraysHandle<value_types...>::~StableDynamicStructOfArraysHandle()
    {
        Free();
    }

    template<typename... value_types>
    inline auto StableDynamicStructOfArraysHandle<value_types...>::operator=(
        StableDynamicStructOfArraysHandle&& other)
        -> StableDynamicStructOfArraysHandle&
    {
        if (this != static_cast<void*>(&other))
        {
            Free();
            m_data = other.m_data;
            m_destructorCallback = other.m_destructorCallback;
            m_page = other.m_page;
            other.Invalidate();
        }
        return *this;
    }

    template<typename... value_types>
    inline void StableDynamicStructOfArraysHandle<value_types...>::Free()
    {
        if (IsValid())
        {
            m_destructorCallback(this);
        }
    }

    template<typename... value_types>
    inline bool StableDynamicStructOfArraysHandle<value_types...>::IsValid() const
    {
        return AZStd::get<0>(m_data) != nullptr;
    }

    template<typename... value_types>
    inline bool StableDynamicStructOfArraysHandle<value_types...>::IsNull() const
    {
        return AZStd::get<0>(m_data) == nullptr;
    }

    template<typename... value_types>
    inline typename StableDynamicStructOfArraysHandle<value_types...>::ItemTupleType&
        StableDynamicStructOfArraysHandle<value_types...>::operator*()
    {
        return m_data;
    }

    template<typename... value_types>
    template<size_t RowIndex>
    inline auto& StableDynamicStructOfArraysHandle<value_types...>::GetItem() const
    {
        return *AZStd::get<RowIndex>(m_data);
    }

    template <typename... value_types>
    inline void StableDynamicStructOfArraysHandle<value_types...>::Invalidate()
    {
        AZStd::apply(
            // We take a reference to the dataItem pointers here, since we're modifying the pointers
            // themselves not the data they're pointing to
            [](value_types *&... dataItems)
            {
                ((dataItems = nullptr), ...);
            },
            m_data
        );

        m_destructorCallback = nullptr;
        m_page = nullptr;
    }

    template<typename... value_types>
    StableDynamicStructOfArraysWeakHandle<value_types...> StableDynamicStructOfArraysHandle<value_types...>::GetWeakHandle() const
    {
        return StableDynamicStructOfArraysWeakHandle<value_types...>(m_data);
    }

} // end namespace AZ
