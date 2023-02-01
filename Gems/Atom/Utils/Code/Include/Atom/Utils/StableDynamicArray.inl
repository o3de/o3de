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

    // StableDynamicArray

    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::StableDynamicArray(allocator_type allocator)
        : m_allocator(allocator)
    {}

    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::~StableDynamicArray()
    {
        // Deallocate the pages and check for allocated items since that may mean there are
        // outstanding handles that we should warn the user about.

        size_t occupiedPageCount = 0;
        size_t orphanedItemCount = 0;

        for (Page* page : m_pages)
        {
            // Need to check for null pages, since there can be holes in m_pages if ReleaseEmptyPages is called
            if (page && !page->IsEmpty())
            {
                ++occupiedPageCount;
                orphanedItemCount += page->GetItemCount();
            }

            m_allocator.deallocate(page, sizeof(Page), AZStd::alignment_of<Page>::value);
        }
        m_pages.clear();

        AZ_Warning("StableDynamicArray", occupiedPageCount == 0,
            "StableDynamicArray is being deleted but there are still %zu outstanding handles on %zu pages. Handles that "
            "are not freed before StableDynamicArray is removed will point to garbage memory.",
            orphanedItemCount, occupiedPageCount
        );
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::insert(const value_type& value) -> Handle
    {
        return emplace(value);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::insert(value_type&& value) -> Handle
    {
        return emplace(AZStd::move(value));
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    template<class ... Args>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::emplace(Args&& ... args)->Handle
    {
        // Try to find a page we can fit this in.
        for (m_firstAvailablePage; m_firstAvailablePage < m_pages.size(); ++m_firstAvailablePage)
        {
            if (m_pages[m_firstAvailablePage])
            {
                size_t pageItem = m_pages[m_firstAvailablePage]->Reserve();
                if (pageItem != Page::InvalidPage)
                {
                    // pageItem is a valid item that's been reserved, so construct a new T on it.
                    T* item = m_pages[m_firstAvailablePage]->GetItem(pageItem);
                    AZStd::Internal::construct<T*>::single(item, AZStd::forward<Args>(args)...);

                    ++m_itemCount;
                    return Handle(
                        item, m_pages[m_firstAvailablePage], static_cast<uint16_t>(m_firstAvailablePage), static_cast<uint16_t>(pageItem));
                }
            }
        }

        // No page to emplace in, so make a new page
        Page* page = AddPage();

        // A new page was created since there was no room in any other page, so this new page will also be the first page where slots are available.
        m_firstAvailablePage = m_pages.size() - 1;

        size_t pageItem = page->Reserve();
        T* item = m_pages[m_firstAvailablePage]->GetItem(pageItem);
        AZStd::Internal::construct<T*>::single(item, AZStd::forward<Args>(args) ...);
        ++m_itemCount;
        return Handle(item, page, static_cast<uint16_t>(m_firstAvailablePage), static_cast<uint16_t>(pageItem));
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::erase(Handle& handle)
    {
        if (!handle.IsValid())
        {
            return;
        }

        // Update the first free page if the page this item is being removed from is earlier in the list.
        if (handle.m_weakHandle.m_pageIndex < m_firstAvailablePage)
        {
            m_firstAvailablePage = handle.m_weakHandle.m_pageIndex;
        }

        // Destroy the data in the handle, invalidate the handle, and free the spot that it points to.
        handle.m_data->~T();
        m_pages[handle.m_weakHandle.m_pageIndex]->Free(handle.m_data);
        handle.Invalidate();
        --m_itemCount;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    size_t StableDynamicArray<T, ElementsPerPage, Allocator>::size() const
    {
        return m_itemCount;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::GetParallelRanges() -> AZStd::vector<AZStd::pair<pageIterator, pageIterator>>
    {
        AZStd::vector<AZStd::pair<pageIterator, pageIterator>> pageIterators;
        pageIterators.reserve(m_pages.size());
        for (Page* page : m_pages)
        {
            // Need to check for null pages, since there can be holes in m_pages if ReleaseEmptyPages is called
            if (page && !page->IsEmpty())
            {
                pageIterators.push_back({ pageIterator(page), pageIterator(nullptr) });
            }
        }
        return pageIterators;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::DefragmentHandle(Handle& handle)
    {
        if (!handle.IsValid() || reinterpret_cast<Page*>(handle.m_page)->IsFull())
        {
            // If this handle has memory in a full page, it's already compact.
            return;
        }

        // Try to find a page we can fit this in.
        for (m_firstAvailablePage; m_firstAvailablePage < m_pages.size(); ++m_firstAvailablePage)
        {
            // if the first page with space available is the page this item is already in, there's not a better page to be in so let it be.
            if (m_pages[m_firstAvailablePage] == handle.m_page)
            {
                break;
            }

            if (m_pages[m_firstAvailablePage])
            {
                size_t pageItemIndex = m_pages[m_firstAvailablePage]->Reserve();
                if (pageItemIndex != Page::InvalidPage)
                {
                    // Found a better page, move the data to it.
                    *m_pages[m_firstAvailablePage]->GetItem(pageItemIndex) = AZStd::move(*handle);
                    reinterpret_cast<Page*>(handle.m_page)->Free(handle.m_data);
                    handle.m_data = m_pages[m_firstAvailablePage]->GetItem(pageItemIndex);
                    handle.m_page = m_pages[m_firstAvailablePage];
                    handle.m_weakHandle = WeakHandle{ static_cast<uint16_t>(m_firstAvailablePage), static_cast<uint16_t>(pageItemIndex) };
                    break;
                }
            }
        }

    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::ReleaseEmptyPages()
    {
        for (size_t pageIndex = 0; pageIndex < m_pages.size(); ++pageIndex)
        {
            // Need to check for null pages, since there can be holes in m_pages if ReleaseEmptyPages is called
            if (m_pages[pageIndex] && m_pages[pageIndex]->IsEmpty())
            {
                m_allocator.deallocate(m_pages[pageIndex], sizeof(Page), AZStd::alignment_of<Page>::value);
                m_pages[pageIndex] = nullptr;
            }
        }

        // Recalculate the first available page.
        for (m_firstAvailablePage = 0; m_firstAvailablePage < m_pages.size(); ++m_firstAvailablePage)
        {
            // Need to check for null pages, since there can be holes in m_pages if ReleaseEmptyPages is called
            if (m_pages[m_firstAvailablePage] && !m_pages[m_firstAvailablePage]->IsFull())
            {
                break;
            }
        }
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArrayMetrics StableDynamicArray<T, ElementsPerPage, Allocator>::GetMetrics()
    {
        StableDynamicArrayMetrics metrics;

        for (Page* page : m_pages)
        {
            // Need to check for null pages, since there can be holes in m_pages if ReleaseEmptyPages is called
            if (page)
            {
                size_t itemCount = page->GetItemCount();
                metrics.m_totalElements += itemCount;
                metrics.m_elementsPerPage.push_back(itemCount);
                if (itemCount == 0)
                {
                    ++metrics.m_emptyPages;
                }
            }
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

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::begin() -> iterator
    {
        if (!m_pages.empty())
        {
            return iterator(m_pages[0]);
        }

        return iterator(nullptr);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::cbegin() const -> const_iterator
    {
        if (!m_pages.empty())
        {
            return const_iterator(m_pages[0]);
        }

        return const_iterator(nullptr);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::end() -> iterator
    {
        return iterator();
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::cend() const -> const_iterator
    {
        return const_iterator();
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::AddPage()->Page*
    {
        void* pageMemory = m_allocator.allocate(sizeof(Page), AZStd::alignment_of<Page>::value);
        Page* page = new (pageMemory) Page();
        page->m_container = this;
        m_pages.push_back(page);
        return page;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    T& StableDynamicArray<T, ElementsPerPage, Allocator>::GetData(WeakHandle handle) const
    {
        AZ_Assert(handle.IsValid() && m_pages[handle.m_pageIndex], "StableDynamicArray<>::GetData called with invalid handle.");
        return *m_pages[handle.m_pageIndex]->GetItem(handle.m_elementIndex);
    }

    // StableDynamicArray::Page


    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::Page::Page()
    {
        m_bits.fill(0);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    size_t StableDynamicArray<T, ElementsPerPage, Allocator>::Page::Reserve()
    {
        for (; m_bitStartIndex < NumUint64_t; ++m_bitStartIndex)
        {
            if (m_bits[m_bitStartIndex] != FullBits)
            {
                // Find the free slot, mark it, and return the index.
                uint64_t freeSlot = az_ctz_u64(~m_bits[m_bitStartIndex]);
                m_bits[m_bitStartIndex] |= 1ull << freeSlot;
                ++m_itemCount;

                return static_cast<size_t>(freeSlot + 64 * m_bitStartIndex);
            }
        }
        return InvalidPage;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::Page::Free(T* item)
    {
        // use the difference between the item pointer and the page data to find the index
        size_t index = item - reinterpret_cast<T*>(&m_data);
        // This item's flag will be in the uint64_t at index >> 6 (index / 64). Mark the appropriate bit as 0 (Free).
        AZ_Assert(m_bits[index >> 6] & (1ull << (index & 0x3F)), "Freeing item that is already marked as free!"); // The IsValid() check on handles should prevent this ever happening.
        m_bits[index >> 6] &= ~(1ull << (index & 0x3F));
        // Set the bit start index so the next Reserve() starts on a uint_64 that likely has space.
        m_bitStartIndex = AZStd::min(index >> 6, m_bitStartIndex);

        --m_itemCount;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::Page::IsFull() const
    {
        return m_itemCount == ElementsPerPage;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::Page::IsEmpty() const
    {
        return m_itemCount == 0;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    T* StableDynamicArray<T, ElementsPerPage, Allocator>::Page::GetItem(size_t index)
    {
        return reinterpret_cast<T*>(&m_data) + index;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    size_t StableDynamicArray<T, ElementsPerPage, Allocator>::Page::GetItemCount() const
    {
        return m_itemCount;
    }


    // StableDynamicArray::iterator


    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::iterator(Page* firstPage)
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

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator*() const -> reference
    {
        return *m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator->() const -> pointer
    {
        return m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator==(const this_type& rhs) const
    {
        return rhs.m_item == m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator++() -> this_type &
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

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::SkipEmptyPages()
    {
        // skip all initial empty pages.
        while (m_page && m_page->IsEmpty())
        {
            m_page = m_page->m_nextPage;
        }

        // If the page is null, it's at the end. This sets m_item to nullptr so that it == StableDynamicArray::End().
        if (m_page == nullptr)
        {
            m_item = nullptr;
            return false;
        }

        // skip the empty bitfields in the page
        for (; m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0; ++m_bitGroupIndex)
        {
        }

        return true;
    }


    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::iterator::AdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_item = m_page->GetItem(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // StableDynamicArray::const_iterator

    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator::const_iterator(Page* firstPage)
        : base_type(firstPage)
    {
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator::operator*() const -> reference
    {
        return *base_type::m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator::operator->() const -> pointer
    {
        return base_type::m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator::operator++() -> this_type &
    {
        base_type::operator++();
        return *this;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    // StableDynamicArray::pageIterator

    template<typename T, size_t ElementsPerPage, class Allocator>
    StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::pageIterator(Page* page)
        : m_page(page)
    {
        if (m_page != nullptr)
        {
            // SkipEmptyBitGroups() will skip all the initial empty bit groups that may exist in the page.
            SkipEmptyBitGroups();
        }
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator*() const -> reference
    {
        return *m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator->() const -> pointer
    {
        return m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator==(const this_type& rhs) const
    {
        return rhs.m_item == m_item;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    bool StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator!=(const this_type& rhs) const
    {
        return !operator==(rhs);
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator++() -> this_type &
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

    template<typename T, size_t ElementsPerPage, class Allocator>
    auto StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::operator++(int) -> this_type
    {
        this_type temp = *this;
        ++this;
        return temp;
    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::SkipEmptyBitGroups()
    {
        // skip the next bit group in the page until one is found with entries
        while (m_bitGroupIndex < Page::NumUint64_t && m_page->m_bits.at(m_bitGroupIndex) == 0)
        {
            ++m_bitGroupIndex;
        }

        if (m_bitGroupIndex >= Page::NumUint64_t)
        {
            // Done with this page, so it's at the end of the page iterator.
            m_item = nullptr;
            return;
        }

        // Set up the bit group from the index found earlier.
        m_remainingBitsInBitGroup = m_page->m_bits.at(m_bitGroupIndex);

        // Set up the item pointer and increments the bits.
        SetItemAndAdvanceIterator();

    }

    template<typename T, size_t ElementsPerPage, class Allocator>
    void StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator::SetItemAndAdvanceIterator()
    {
        uint64_t index = az_ctz_u64(m_remainingBitsInBitGroup);
        m_item = m_page->GetItem(m_bitGroupIndex * 64 + index);

        // Lop off the lowest bit to prepare for forward iteration.
        m_remainingBitsInBitGroup &= (m_remainingBitsInBitGroup - 1);
    }

    // StableDynamicArray::Handle


    template<typename ValueType>
    template<typename PageType>
    StableDynamicArrayHandle<ValueType>::StableDynamicArrayHandle(ValueType* data, PageType* page, uint16_t pageIndex, uint16_t elementIndex)
        : m_data(data)
        , m_page(page)
        , m_weakHandle(pageIndex, elementIndex)
    {
        // Store container type information in the non-capturing lambda callback so the Handle itself doesn't need it.
        m_destructorCallback = [](void* typelessHandlePointer)
        {
            StableDynamicArrayHandle* handle = static_cast<StableDynamicArrayHandle*>(typelessHandlePointer);
            static_cast<PageType*>(handle->m_page)->m_container->erase(*handle);
        };
    }

    template<typename ValueType>
    StableDynamicArrayHandle<ValueType>::StableDynamicArrayHandle(StableDynamicArrayHandle&& other)
    {
        *this = AZStd::move(other);
    }

    template <typename ValueType>
    template <typename OtherType>
    StableDynamicArrayHandle<ValueType>::StableDynamicArrayHandle(StableDynamicArrayHandle<OtherType>&& other)
    {
        *this = AZStd::move(other);
    }

    template<typename ValueType>
    StableDynamicArrayHandle<ValueType>::~StableDynamicArrayHandle()
    {
        Free();
    }

    template<typename ValueType>
    auto StableDynamicArrayHandle<ValueType>::operator=(StableDynamicArrayHandle&& other) -> StableDynamicArrayHandle&
    {
        if (this != static_cast<void*>(&other))
        {
            Free();
            m_data = other.m_data;
            m_destructorCallback = other.m_destructorCallback;
            m_page = other.m_page;
            m_weakHandle = other.m_weakHandle;
            other.Invalidate();
        }
        return *this;
    }

    template <typename ValueType>
    template <typename OtherType>
    auto StableDynamicArrayHandle<ValueType>::operator=(StableDynamicArrayHandle<OtherType>&& other) -> StableDynamicArrayHandle<ValueType>&
    {
        static_assert((AZStd::is_base_of<ValueType, OtherType>::value || AZStd::is_base_of<OtherType, ValueType>::value), "Cannot move a StableDynamicArrayHandle to a handle of an unrelated type.");

        Free();
        m_data = azrtti_cast<ValueType*>(other.m_data);
        // Only move the data if the azrtti_cast cast succeeded. Otherwise, leave both handles invalid
        if (m_data)
        {
            m_page = other.m_page;
            m_weakHandle = StableDynamicArrayWeakHandle<ValueType>{ other.m_weakHandle.m_pageIndex, other.m_weakHandle.m_elementIndex };

            // The destructor callback is a non-capturing lambda, which has no state and can be used as a plain function.
            // Because the lambda is is created when the original handle is constructed, it captures the underlying type the handle refers to
            // even if the handle is being moved from BaseClass handle to a DerivedClass handle or vice versa
            m_destructorCallback = other.m_destructorCallback;
        }
        else if (other.m_data)
        {
            // If the cast failed, assert here because something is trying to cast between unrelated handle types
            AZ_Assert(false, "StableDynamicArrayHandle: Failed to azrtti_cast from %s to %s", OtherType::RTTI_TypeName(), ValueType::RTTI_TypeName());

            // Since we're about to invalidate the other handle, but this handle won't be assuming ownership, free the object referred to by the other handle
            other.Free();
        }

        other.Invalidate();

        return *this;
    }

    template<typename ValueType>
    void StableDynamicArrayHandle<ValueType>::Free()
    {
        if (IsValid())
        {
            m_destructorCallback(this);
        }
    }

    template<typename ValueType>
    bool StableDynamicArrayHandle<ValueType>::IsValid() const
    {
        return m_data != nullptr;
    }

    template<typename ValueType>
    bool StableDynamicArrayHandle<ValueType>::IsNull() const
    {
        return m_data == nullptr;
    }

    template<typename ValueType>
    ValueType& StableDynamicArrayHandle<ValueType>::operator*() const
    {
        return *m_data;
    }

    template<typename ValueType>
    ValueType* StableDynamicArrayHandle<ValueType>::operator->() const
    {
        return m_data;
    }

    template<typename ValueType>
    void StableDynamicArrayHandle<ValueType>::Invalidate()
    {
        m_data = nullptr;
        m_destructorCallback = nullptr;
        m_page = nullptr;
        m_weakHandle = StableDynamicArrayWeakHandle<ValueType>{};
    }

    template<typename ValueType>
    StableDynamicArrayWeakHandle<ValueType> StableDynamicArrayHandle<ValueType>::GetWeakHandle() const
    {
        return m_weakHandle;
    }

} // end namespace AZ
