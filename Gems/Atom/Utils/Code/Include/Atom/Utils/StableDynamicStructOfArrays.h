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
    struct StableDynamicStructOfArraysMetrics;

    template<typename... value_types>
    class StableDynamicStructOfArraysWeakHandle;
    
    template<typename... value_types>
    class StableDynamicStructOfArraysHandle;

    /**
    *   A StableDynamicStructOfArrays uses a variable number of arrays to store data. Basically this container
    * is a list of arrays, with some information to track usage within those arrays, some
    * optimization to keep jumping through the list to a minimum, and a forward iterator to traverse
    * the whole container.
    *   This StructOfArrays version differs from StableDynamicArray in that instead of taking a single type, it
    * takes a parameter pack of types, where each type is in a different 'row' of the StructOfArrays. This allows the data to be split
    * into multiple rows, to keep the size of the data in each row smaller for better cache coherencey when iterating.
    *   This container produces better cache locality when iterating on elements (vs a list) and keeps 
    * appending/removing cost low by reusing empty slots. Resizing is also contained to allocating new
    * arrays.
    *   It will always place new items at the front-most slot of the first array with available space.
    * DefragmentHandle() can be called to reorganize data to reduce the amount of empty slots.
    **/

    // The template paramaterization is a little different than StableDynamicArray. The ElementsPerPage and Allocator
    // come first, and do not have default arguments, so that it is explicit which arguments are part of the parameter pack (...)
    // see https://en.cppreference.com/w/cpp/language/parameter_pack
    // "In a primary class template, the template parameter pack must be the final parameter in the template parameter list."
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    class StableDynamicStructOfArrays
    {
        static_assert(!(ElementsPerPage % 64) && ElementsPerPage > 0, "ElementsPerPage must be a multiple of 64.");

        class iterator;
        class const_iterator;
        class pageIterator;

        friend iterator;
        friend const_iterator;
        friend StableDynamicStructOfArraysHandle<value_types...>;

        typedef Allocator allocator_type;

        struct Page;

    public:

        using Handle = StableDynamicStructOfArraysHandle<value_types...>;
        using WeakHandle = StableDynamicStructOfArraysWeakHandle<value_types...>;
        using ItemTupleType = AZStd::tuple<value_types*...>;

        StableDynamicStructOfArrays() = default;
        explicit StableDynamicStructOfArrays(allocator_type allocator);
        ~StableDynamicStructOfArrays();

        /// Reserves and constructs an item of type T and returns a handle to it.
        Handle insert(const value_types&... value);

        /// Reserves items of type value_types, forwards the provided args to construct the items, and returns a handle to it.
        /// Can be called with AZStd::forward_as_tuple to collect the arguments for each item into tuples.
        template<typename... TupleArgs>
        Handle emplace(TupleArgs&&... args);

        /// Destructs and frees the memory associated with a handle, then invalidates the handle.
        void erase(Handle& handle);

        /// Returns the number of items in this container.
        size_t size() const;

        /*
         * Returns pairs of begin and end iterators for that represent contiguous ranges of elements 
         * in the StableDynamicStructOfArrays. This is useful for cases where all of the items in the
         * StableDynamicStructOfArrays can be processsed in parallel by iterating through each range on a 
         * different thread. Since StableDynamicStructOfArrays only uses forward iterators, this would be
         * expensive to create external to this class.
         *
         * The iterators themselves operate using the bitmask representing used slots in the page, but
         * they expose access to any row of a page via GetItem<RowIndex>().
         */
        AZStd::vector<AZStd::pair<pageIterator, pageIterator>> GetParallelRanges();

        /* 
        * If the memory associated with this handle can be moved to a more compact spot, it will be.
        * This will change the pointer inside the handle, so should only be called when no other system
        * is holding on to a direct pointer to the same memory, such as a WeakHandle
        */
        void DefragmentHandle(Handle& handle);

        /// Release any empty pages that may exist to free up memory.
        void ReleaseEmptyPages();

        /*
        * Returns information about the state of the container, like how many pages are allocated and 
        * how compact they are.
        */
        StableDynamicStructOfArraysMetrics GetMetrics();

        /// Returns a forward iterator to the start of the array
        iterator begin();
        const_iterator cbegin() const;

        /// Returns an iterator representing the end of the array.
        iterator end();
        const_iterator cend() const;

    private:

        template<class... TupleArgs>
        Handle EmplaceTupleElements(Page* page, size_t elementIndex, TupleArgs&&... tupleArgs);

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

    /// Private class used by StableDynamicStructOfArrays to manage the arrays of data.
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    struct StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::Page
    {
        static constexpr size_t InvalidPage = AZStd::numeric_limits<size_t>::max();
        static constexpr uint64_t FullBits = 0xFFFFFFFFFFFFFFFFull;
        static constexpr size_t NumUint64_t = ElementsPerPage / 64;
        using ItemTupleType = AZStd::tuple<value_types *...>;
        Page();
        ~Page() = default;

        /// Reserve the next availble index and return it. If no more space is available, returns InvalidPage.
        size_t Reserve();

        /// Free the given index so it can be reserved again.
        void Free(ItemTupleType items);

        /// True if this page is completely full
        bool IsFull() const;

        /// True if this page is completely empty
        bool IsEmpty() const;

        ItemTupleType GetItems(size_t index);

        /**
        * Gets a specific item from a specific row of the Page
        * Note: may return empty slots.
        */
        template<size_t RowIndex>
        auto* GetItem(size_t index);

        /// Gets the number of items allocated on this page.
        size_t GetItemCount() const;

        size_t m_bitStartIndex = 0; ///< Index of the first uint64_t that might have space.
        Page* m_nextPage = nullptr; ///< pointer to the next page.
        StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>*
            m_container; ///< pointer to the container this page was allocated from.
        size_t m_pageIndex = 0; ///< used for comparing pages when items are freed so the earlier page in the list can be cached.
        size_t m_itemCount = 0; ///< the number of items in the page.
        AZStd::array<uint64_t, NumUint64_t> m_bits; ///< Bits representing free slots in the array. Free slots are 1, occupied slots are 0.

        using PageTupleType = AZStd::tuple<AZStd::aligned_storage_t<ElementsPerPage * sizeof(value_types), alignof(value_types)>...>;
        PageTupleType m_data; ///< aligned storage for all the actual data.
    };

    /// Forward iterator for StableDynamicStructOfArrays
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    class StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator
    {
        using this_type = iterator;
        using container_type = StableDynamicStructOfArrays;

    public:
        using iterator_category = AZStd::forward_iterator_tag;

        iterator() = default;
        explicit iterator(Page* firstPage);

        const this_type& operator*() const;
        template <size_t RowIndex>
        auto& GetItem() const;

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
        size_t m_itemIndex = AZStd::numeric_limits<size_t>::max(); ///< The index to the current item

    };

    /// Forward const iterator for StableDynamicStructOfArrays
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    class StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::const_iterator
        : public StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::iterator
    {
        using this_type = const_iterator;
        using base_type = iterator;

    public:

        const_iterator() = default;
        explicit const_iterator(Page* firstPage);

        template<size_t RowIndex>
        auto& GetItem() const;

        this_type& operator++();
        this_type operator++(int);
    };

    /// Forward iterator for an individual page in StableDynamicStructOfArrays
    template<size_t ElementsPerPage, class Allocator, typename... value_types>
    class StableDynamicStructOfArrays<ElementsPerPage, Allocator, value_types...>::pageIterator
    {
        using this_type = pageIterator;
        using container_type = StableDynamicStructOfArrays;

    public:
        using iterator_category = AZStd::forward_iterator_tag;

        pageIterator() = default;
        explicit pageIterator(Page* page);

        const this_type& operator*() const;
        template<size_t RowIndex>
        auto& GetItem() const;

        bool operator==(const this_type& rhs) const;
        bool operator!=(const this_type& rhs) const;

        this_type& operator++();
        this_type operator++(int);

    private:

        void SkipEmptyBitGroups();
        void SetItemAndAdvanceIterator();

        Page* m_page = nullptr; ///< Pointer to the current page being iterrated through
        size_t m_bitGroupIndex = 0; ///< The index of the current bit group in the m_page
        uint64_t m_remainingBitsInBitGroup = 0; ///< This starts out equivalent to the bits from the current bit group, but trailing 1s are changed to 0s as the iterator increments
        size_t m_itemIndex = AZStd::numeric_limits<size_t>::max(); ///< The index to the current item

    };

    /**
    * A weak reference to the data allocated in the array. It can be copied, and will not auto-release the data
    * when it goes out of scope. There is no guarantee that a weak handle is not dangling, so it should only be used
    * in cases where it is known that the owning handle has not gone out of scope. This could potentially be augmented in the future
    * to have a salt/generationId that could be used to determine if it is a dangling reference.
    */
    template<typename... value_types>
    class StableDynamicStructOfArraysWeakHandle
    {
    public:
        using ItemTupleType = AZStd::tuple<value_types*...>;

        /// Default constructor creates an invalid handle.
        StableDynamicStructOfArraysWeakHandle() = default;

        /// Returns true if this Handle currently holds a valid value.
        bool IsValid() const;

        /// Returns true if this Handle doesn't contain a value (same as !IsValid()).
        bool IsNull() const;

        ItemTupleType& operator*();

        template <size_t RowIndex>
        auto& GetItem() const;
        
    private:
        StableDynamicStructOfArraysWeakHandle(const ItemTupleType& data);

        ItemTupleType m_data;

        template<typename... value_types>
        friend class StableDynamicStructOfArraysHandle;
    };

    /**
    * Handle to the data allocated in the array. This stores extra data internally so that an item can be
    * quickly marked as free later. Since there is no ref counting, copy is not allowed, only move. When
    * a handle is used to free it's associated data it is marked as invalid.
    */
    template <typename... value_types>
    class StableDynamicStructOfArraysHandle
    {
        template<size_t ElementsPerPage, class Allocator, typename... value_types>
        friend class StableDynamicStructOfArrays;
    public:
        using ItemTupleType = AZStd::tuple<value_types*...>;

        /// Default constructor creates an invalid handle.
        StableDynamicStructOfArraysHandle() = default;

        /// Move Constructor
        StableDynamicStructOfArraysHandle(StableDynamicStructOfArraysHandle&& other);
        
        /// Destructor will also destroy its underlying data and free it from the StableDynamicStructOfArrays
        ~StableDynamicStructOfArraysHandle();

        /// Move Assignment
        StableDynamicStructOfArraysHandle& operator=(StableDynamicStructOfArraysHandle&& other);

        /// Destroy the underlying data and free it from the StableDynamicStructOfArrays. Marks the handle as invalid.
        void Free();

        /// Returns true if this Handle currently holds a valid value.
        bool IsValid() const;

        /// Returns true if this Handle doesn't contain a value (same as !IsValid()).
        bool IsNull() const;

        ItemTupleType& operator*();

        /// Returns a pointer to the item in the row specified by RowIndex
        template <size_t RowIndex>
        auto& GetItem() const;

        /// Returns a non-owning weak handle to the data
        StableDynamicStructOfArraysWeakHandle<value_types...> GetWeakHandle() const;
    private:
        template<typename PageType>
        StableDynamicStructOfArraysHandle(ItemTupleType data, PageType* page);

        StableDynamicStructOfArraysHandle(const StableDynamicStructOfArraysHandle&) = delete;

        void Invalidate();

        using HandleDestructor = void (*)(void*);
        /// Called for valid handles on delete so the underlying data can be removed from the StableDynamicArray
        HandleDestructor m_destructorCallback = nullptr; 
        void* m_page = nullptr; ///< The page the data this Handle points to was allocated on.
        ItemTupleType m_data;
    };
    
    /// Used for returning information about the internal state of the StableDynamicStructOfArrays.
    struct StableDynamicStructOfArraysMetrics
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

#include<Atom/Utils/StableDynamicStructOfArrays.inl>
