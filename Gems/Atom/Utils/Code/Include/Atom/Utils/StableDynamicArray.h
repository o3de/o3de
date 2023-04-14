/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/MathIntrinsics.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/typetraits/aligned_storage.h>
#include <AzCore/base.h>
#include <stdint.h>

namespace AZ
{
    /// forward declarations
    struct StableDynamicArrayMetrics;

    template<typename ValueType>
    class StableDynamicArrayWeakHandle;

    template<typename ValueType>
    class StableDynamicArrayHandle;

    /**
    *   A StableDynamicArray uses a variable number of arrays to store data. Basically this container
    * is a list of arrays, with some information to track usage within those arrays, some
    * optimization to keep jumping through the list to a minimum, and a forward iterator to traverse
    * the whole container.
    *   This container produces better cache locality when iterating on elements (vs a list) and keeps 
    * appending/removing cost low by reusing empty slots. Resizing is also contained to allocating new
    * arrays.
    *   It will always place new items at the front-most slot of the first array with available space.
    * DefragmentHandle() can be called to reorganize data to reduce the amount of empty slots.
    **/
    template<typename T, size_t ElementsPerPage = 512, class Allocator = AZStd::allocator>
    class StableDynamicArray
    {
        static_assert(!(ElementsPerPage % 64) && ElementsPerPage > 0, "PageSize must be a multiple of 64.");
        static constexpr size_t PageSize = ElementsPerPage * sizeof(T);

        using value_type = T;

        class iterator;
        class const_iterator;

        friend iterator;
        friend const_iterator;
        friend StableDynamicArrayHandle<T>;

        typedef Allocator allocator_type;

        struct Page;

    public:

        class pageIterator;
        struct IteratorRange
        {
            pageIterator m_begin;
            pageIterator m_end;
        };
        using ParallelRanges = AZStd::vector<IteratorRange>;
        using Handle = StableDynamicArrayHandle<T>;
        using WeakHandle = StableDynamicArrayWeakHandle<T>;

        StableDynamicArray() = default;
        explicit StableDynamicArray(allocator_type allocator);
        ~StableDynamicArray();

        /// Reserves and constructs an item of type T and returns a handle to it.
        Handle insert(const value_type& value);
        /// Reserves and constructs an item of type T and returns a handle to it.
        Handle insert(value_type&& value);

        /// Reserves and constructs an item of type T with provided args and returns a handle to it.
        template<class ... Args>
        Handle emplace(Args&& ... args);

        /// Destructs and frees the memory associated with a handle, then invalidates the handle.
        void erase(Handle& handle);

        /// Returns the number of items in this container.
        size_t size() const;

        /*
         * Returns pairs of begin and end iterators for that represent contiguous ranges of elements 
         * in the StableDynamicArray. This is useful for cases where all of the items in the
         * StableDynamicArray can be processsed in parallel by iterating through each range on a 
         * different thread. Since StableDynamicArray only uses forward iterators, this would be
         * expensive to create external to this class.
         */
        ParallelRanges GetParallelRanges();

        /* 
        * If the memory associated with this handle can be moved to a more compact spot, it will be.
        * This will change the pointer inside the handle, so should only be called when no other system
        * is holding on to a direct pointer to the same memory.
        */
        void DefragmentHandle(Handle& handle);

        /// Release any empty pages that may exist to free up memory.
        void ReleaseEmptyPages();

        /*
        * Returns information about the state of the container, like how many pages are allocated and 
        * how compact they are.
        */
        StableDynamicArrayMetrics GetMetrics();

        /// Returns a forward iterator to the start of the array
        iterator begin();
        const_iterator cbegin() const;

        /// Returns an iterator representing the end of the array.
        iterator end();
        const_iterator cend() const;

        /// Returns the page index for the given handle
        size_t GetPageIndex(const Handle& handle) const;

    private:

        /// Adds a page and returns its pointer
        Page* AddPage();

        allocator_type m_allocator;
        Page* m_firstPage = nullptr; ///< First page in the list of pages

        /// Used as an optimization to skip pages that are known to already be full. Generally this will point a page that has space available
        /// in it, but it could point to a full page as long as there are no other available pages before that full page. When there are no
        /// pages at all, this will point to nullptr. When all pages are full, this may point to any page, including the last page.
        Page* m_firstAvailablePage = nullptr;

        size_t m_pageCounter = 0; ///< The total number of pages that have been created (not how many currently exist).
        size_t m_itemCount = 0; ///< The total number of items in this container.
    };

    /// Private class used by StableDynamicArray to manage the arrays of data.
    template<typename T, size_t ElementsPerPage, class Allocator>
    struct StableDynamicArray<T, ElementsPerPage, Allocator>::Page
    {
        static constexpr size_t InvalidPage = std::numeric_limits<size_t>::max();
        static constexpr uint64_t FullBits = 0xFFFFFFFFFFFFFFFFull;
        static constexpr size_t NumUint64_t = ElementsPerPage / 64;

        Page();
        ~Page() = default;

        /// Reserve the next availble index and return it. If no more space is available, returns InvalidPage.
        size_t Reserve();

        /// Free the given index so it can be reserved again.
        void Free(T* item);

        /// True if this page is completely full
        bool IsFull() const;

        /// True if this page is completely empty
        bool IsEmpty() const;

        /**
        * Gets a specific item from the Page
        * Note: may return empty slots.
        */
        T* GetItem(size_t index);

        /// Gets the number of items allocated on this page.
        size_t GetItemCount() const;

        size_t m_bitStartIndex = 0; ///< Index of the first uint64_t that might have space.
        Page* m_nextPage = nullptr; ///< pointer to the next page.
        StableDynamicArray<T, ElementsPerPage, Allocator>* m_container; ///< pointer to the container this page was allocated from.
        size_t m_pageIndex = 0; ///< used for comparing pages when items are freed so the earlier page in the list can be cached.
        size_t m_itemCount = 0; ///< the number of items in the page.
        AZStd::array<uint64_t, NumUint64_t> m_bits; ///< Bits representing free slots in the array. Free slots are 1, occupied slots are 0.
        AZStd::aligned_storage_t<PageSize, alignof(T)> m_data; ///< aligned storage for all the actual data.
    };

    /// Forward iterator for StableDynamicArray
    template<typename T, size_t ElementsPerPage, class Allocator>
    class StableDynamicArray<T, ElementsPerPage, Allocator>::iterator
    {
        using this_type = iterator;
        using container_type = StableDynamicArray;

    public:
        using iterator_category = AZStd::forward_iterator_tag;
        using value_type = T;
        using reference = T&;
        using pointer = T*;

        iterator() = default;
        explicit iterator(Page* firstPage);

        reference operator*() const;
        pointer operator->() const;

        bool operator==(const this_type& rhs) const;
        bool operator!=(const this_type& rhs) const;

        this_type& operator++();
        this_type operator++(int);

    protected:

        bool SkipEmptyPages();
        void AdvanceIterator();

        Page* m_page = nullptr; ///< Pointer to the current page being iterrated through
        size_t m_bitGroupIndex = 0; ///< The index of the current bit group in the m_page
        uint64_t m_remainingBitsInBitGroup = 0; ///< This starts out equivalent to the bits from the current bit group, but trailing 1s are changed to 0s as the iterator increments
        T* m_item = nullptr; ///< The pointer to the current item

    };

    /// Forward const iterator for StableDynamicArray
    template<typename T, size_t ElementsPerPage, class Allocator>
    class StableDynamicArray<T, ElementsPerPage, Allocator>::const_iterator
        : public StableDynamicArray<T, ElementsPerPage, Allocator>::iterator
    {
        using this_type = const_iterator;
        using base_type = iterator;

    public:
        using reference = const T & ;
        using pointer = const T * ;

        const_iterator() = default;
        explicit const_iterator(Page* firstPage);

        reference operator*() const;
        pointer operator->() const;

        this_type& operator++();
        this_type operator++(int);
    };

    /// Forward iterator for an individual page in StableDynamicArray
    template<typename T, size_t ElementsPerPage, class Allocator>
    class StableDynamicArray<T, ElementsPerPage, Allocator>::pageIterator
    {
        using this_type = pageIterator;
        using container_type = StableDynamicArray;

    public:
        using iterator_category = AZStd::forward_iterator_tag;
        using value_type = T;
        using reference = T &;
        using pointer = T *;

        pageIterator() = default;
        explicit pageIterator(Page* page);

        reference operator*() const;
        pointer operator->() const;

        bool operator==(const this_type& rhs) const;
        bool operator!=(const this_type& rhs) const;

        this_type& operator++();
        this_type operator++(int);

        size_t GetPageIndex() const;

    private:

        void SkipEmptyBitGroups();
        void SetItemAndAdvanceIterator();

        Page* m_page = nullptr; ///< Pointer to the current page being iterrated through
        size_t m_bitGroupIndex = 0; ///< The index of the current bit group in the m_page
        uint64_t m_remainingBitsInBitGroup = 0; ///< This starts out equivalent to the bits from the current bit group, but trailing 1s are changed to 0s as the iterator increments
        T* m_item = nullptr; ///< The pointer to the current item

    };

    /**
    * A weak reference to the data allocated in the array. It can be copied, and will not auto-release the data
    * when it goes out of scope. There is no guarantee that a weak handle is not dangling, so it should only be used
    * in cases where it is known that the owning handle has not gone out of scope. This could potentially be augmented in the future
    * to have a salt/generationId that could be used to determine if it is a dangling reference.
    */
    template<typename ValueType>
    class StableDynamicArrayWeakHandle
    {
    public:
        StableDynamicArrayWeakHandle() = default;

        /// Returns true if this Handle currently holds a valid value.
        bool IsValid() const;

        /// Returns true if this Handle doesn't contain a value (same as !IsValid()).
        bool IsNull() const;

        ValueType& operator*() const;
        ValueType* operator->() const;

        bool operator==(const StableDynamicArrayWeakHandle<ValueType>& rhs) const;
        bool operator!=(const StableDynamicArrayWeakHandle<ValueType>& rhs) const;
        bool operator<(const StableDynamicArrayWeakHandle<ValueType>& rhs) const;

        
    private:
        StableDynamicArrayWeakHandle(ValueType* data);

        ValueType* m_data = nullptr;

        template<typename T>
        friend class StableDynamicArrayHandle;
    };

    /**
    * Handle to the data allocated in the array. This stores extra data internally so that an item can be
    * quickly marked as free later. Since there is no ref counting, copy is not allowed, only move. When
    * a handle is used to free it's associated data it is marked as invalid.
    */
    template<typename ValueType>
    class StableDynamicArrayHandle
    {
        template<typename T, size_t ElementsPerPage, class Allocator>
        friend class StableDynamicArray;

        template<typename OtherType>
        friend class StableDynamicArrayHandle;
    public:

        /// Default constructor creates an invalid handle.
        StableDynamicArrayHandle() = default;

        /// Move Constructor
        StableDynamicArrayHandle(StableDynamicArrayHandle&& other);

        /// Move constructor that allows converting between handles of different types, as long as they are part of the same inheritance chain
        template<typename OtherType>
        StableDynamicArrayHandle<ValueType>(StableDynamicArrayHandle<OtherType>&& other);
        
        /// Destructor will also destroy its underlying data and free it from the StableDynamicArray
        ~StableDynamicArrayHandle();

        /// Move Assignment
        StableDynamicArrayHandle& operator=(StableDynamicArrayHandle&& other);

        /// Move assignment that allows converting between handles of different types, as long as they are part of the same inheritance chain
        template<typename OtherType>
        StableDynamicArrayHandle<ValueType>& operator=(StableDynamicArrayHandle<OtherType>&& other);

        /// Destroy the underlying data and free it from the StableDynamicArray. Marks the handle as invalid.
        void Free();

        /// Returns true if this Handle currently holds a valid value.
        bool IsValid() const;

        /// Returns true if this Handle doesn't contain a value (same as !IsValid()).
        bool IsNull() const;

        ValueType& operator*() const;
        ValueType* operator->() const;

        /// Returns a non-owning weak handle to the data
        StableDynamicArrayWeakHandle<ValueType> GetWeakHandle() const;

    private:

        template<typename PageType>
        StableDynamicArrayHandle(ValueType* data, PageType* page);

        StableDynamicArrayHandle(const StableDynamicArrayHandle&) = delete;

        void Invalidate();

        using HandleDestructor = void(*)(void*);
        HandleDestructor m_destructorCallback = nullptr; ///< Called for valid handles on delete so the underlying data can be removed from the StableDynamicArray

        ValueType* m_data = nullptr; ///< The actual data this handle points to in the StableDynamicArrayHandle.
        void* m_page = nullptr; ///< The page the data this Handle points to was allocated on.
    };
    
    /// Used for returning information about the internal state of the StableDynamicArray.
    struct StableDynamicArrayMetrics
    {
        AZStd::vector<size_t> m_elementsPerPage;
        size_t m_totalElements = 0;
        size_t m_emptyPages = 0;

        /** 
        * 1.0 = there are no more pages then there needs to be, 0.5 means there are twice as many pages as needed etc.
        * This can be used to help decide if it's worth compacting handles into fewer pages.
        */
        float m_itemToPageRatio = 0.0f;
    };

} // end namespace AZ

#include<Atom/Utils/StableDynamicArray.inl>
