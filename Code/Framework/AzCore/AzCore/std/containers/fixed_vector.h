/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/typetraits/typetraits.h>

namespace AZStd::Internal
{
    template <size_t N>
    using fixed_storage_size_t =
        AZStd::conditional_t<(N < std::numeric_limits<uint8_t>::max()), uint8_t,
        AZStd::conditional_t<(N < std::numeric_limits<uint16_t>::max()), uint16_t,
        AZStd::conditional_t<(N < std::numeric_limits<uint32_t>::max()), uint32_t,
        AZStd::conditional_t<(N < std::numeric_limits<uint32_t>::max()), uint64_t, size_t
        >>>>;

    template<typename T>
    struct fixed_zero_size_storage
    {
        using size_type = uint8_t;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;

        constexpr fixed_zero_size_storage() = default;

        //! Constructs zero sized storage from an initializer list of size zero which is a no-op
        template <typename U, typename = enable_if_t<is_convertible_v<U, T>>>
        constexpr fixed_zero_size_storage(AZStd::initializer_list<U> ilist) noexcept
        {
            AZSTD_CONTAINER_ASSERT(ilist.size() == 0, "Zero size storage is only constructible with an empty initializer list");
        }

        static constexpr pointer data() noexcept
        {
            return {};
        }
        //! Number of elements currently stored.
        static constexpr size_type size() noexcept
        {
            return {};
        }
        //! Capacity of the storage.
        static constexpr size_type capacity() noexcept
        {
            return {};
        }
        //! Is the storage empty?
        static constexpr bool empty() noexcept
        {
            return true;
        }
        //! Is the storage full?
        static constexpr bool full() noexcept
        {
            return true;
        }

        //! Max size that can fit within container
        static constexpr size_type max_size() noexcept
        {
            return 0;
        }

        //! Constructs a new element at the end of the storage
        //! in-place.
        //!
        //! Increases size of the storage by one.
        //! Always fails for zero-sized storage.
        template <typename... Args, typename = enable_if_t<is_constructible_v<T, Args...>>>
        static constexpr void emplace_back(Args&&...) noexcept
        {
            AZSTD_CONTAINER_ASSERT(false, "emplace_back is not supported on zero-size storage");
        }
        //! Removes the last element of the storage.
        //! Always fails for zero-sized storage.
        static constexpr void pop_back() noexcept
        {
            AZSTD_CONTAINER_ASSERT(false, "pop_back is not supported on zero-size storage");
        }
        //! Changes the size of the storage without adding or
        //! removing elements (unsafe).
        //!
        //! The size of an empty storage can only be changed to 0.
        static constexpr void resize_no_construct(size_t new_size) noexcept
        {
            AZSTD_CONTAINER_ASSERT(new_size == 0, "Zero size storage can only be resized to zero");
        }

        //! Invokes destructor on all elements in range
        //! No-op on empty container
        //! Nothing to destroy since the storage is empty.
        template <typename InputIt, typename = enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        static constexpr void unsafe_destroy(InputIt, InputIt) noexcept
        {
        }

        //! Invokes destructor on all elements
        //! No-op on empty container
        static constexpr void unsafe_destroy_all() noexcept
        {
        }

    };

    template<typename T, size_t Capacity>
    struct fixed_trivial_storage
    {
        static_assert(is_trivial_v<T>, "T should be trivial type. Use fixed_non_trivial storage instead");
        static_assert(Capacity, "Capacity cannot be zero. Use fixed_zero_size_storage instead");

        using size_type = fixed_storage_size_t<Capacity>;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;


        //! Constructors
        fixed_trivial_storage() = default;

        template <typename U, typename = enable_if_t<is_convertible_v<U, T>>>
        fixed_trivial_storage(AZStd::initializer_list<U> ilist) noexcept
            : m_size(aznumeric_caster(ilist.size()))
        {
            AZSTD_CONTAINER_ASSERT(ilist.size() <= capacity(), "Initializer list cannot be larger than storage capacity");
            size_t index{};
            for (const U& element : ilist)
            {
                m_data[index++] = element;
            }
        }

        pointer data() noexcept
        {
            return m_data;
        }
        const_pointer data() const noexcept
        {
            return m_data;
        }

        //! Number of elements currently stored.
        size_type size() const noexcept
        {
            return m_size;
        }
        //! Capacity of the storage.
        static constexpr size_type capacity() noexcept
        {
            return Capacity;
        }
        //! Is the storage empty?
        bool empty() const noexcept
        {
            return size() == 0;
        }
        //! Is the storage full?
        bool full() const noexcept
        {
            return size() == capacity();
        }

        //! Max size that can fit within container
        static constexpr size_type max_size() noexcept
        {
            return capacity();
        }

        //! Constructs a new element at the end of the storage
        //! in-place.
        //!
        //! Increases size of the storage by one.
        //! Always fails for empty storage.
        template <typename... Args, typename = enable_if_t<is_constructible_v<T, Args...>>>
        reference emplace_back(Args&&... args) noexcept
        {
            AZSTD_CONTAINER_ASSERT(!full(), "emplace_back cannot be invoked on full storage");
            reference new_element = *(data() + size());
            AZStd::construct_at(&new_element, AZStd::forward<Args>(args)...);
            resize_no_construct(size() + 1);
            return new_element;
        }
        //! Removes the last element of the storage.
        //! Precondition: size is not empty
        void pop_back() noexcept
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "pop_back cannot be invoked on empty storage");
            resize_no_construct(size() - 1);
        }
        //! Changes the size of the storage without adding or
        //! removing elements (unsafe).
        //!
        //! Updates the size of the container while checking that the new size is less than capacity
        void resize_no_construct(size_t new_size) noexcept
        {
            AZSTD_CONTAINER_ASSERT(new_size <= capacity(), "New size cannot be larger than capacity");
            m_size = aznumeric_cast<size_type>(new_size);
        }

        //!  Destructs elements in the range [begin, end).
        //! This does not modify the size of the storage
        //! This is a no-op for trivial types
        template <typename InputIt, typename = enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        void unsafe_destroy(InputIt, InputIt) noexcept
        {
        }

        //! Destructs all elements of the storage.
        //! This does not modify the size of the storage
        //! This is a no-op for trivial types
        void unsafe_destroy_all() noexcept
        {
        }

    private:
        T m_data[Capacity];
        size_type m_size{};
    };

    template<typename T, size_t Capacity>
    struct fixed_non_trivial_storage
    {
        static_assert(!is_trivial_v<T>, "T should not be trivial type. Use fixed_trivial storage instead");
        static_assert(Capacity, "Capacity cannot be zero. Use fixed_zero_size_storage instead");

        using size_type = fixed_storage_size_t<Capacity>;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;

        fixed_non_trivial_storage() = default;

        ~fixed_non_trivial_storage() noexcept
        {
            unsafe_destroy_all();
        }

        template <typename U, typename = enable_if_t<is_convertible_v<U, T>>>
        fixed_non_trivial_storage(AZStd::initializer_list<U> ilist) noexcept(noexcept(emplace_back(AZStd::declval<U>())))
        {
            AZSTD_CONTAINER_ASSERT(ilist.size() <= capacity(), "Initializer list cannot be larger than storage capacity");
            for (const U& element : ilist)
            {
                emplace_back(element);
            }
        }

        pointer data() noexcept
        {
            return reinterpret_cast<pointer>(m_data);
        }
        const_pointer data() const noexcept
        {
            return reinterpret_cast<const_pointer>(m_data);
        }

        //! Number of elements currently stored.
        size_type size() const noexcept
        {
            return m_size;
        }
        //! Capacity of the storage.
        static constexpr size_type capacity() noexcept
        {
            return Capacity;
        }
        //! Is the storage empty?
        bool empty() const noexcept
        {
            return size() == 0;
        }
        //! Is the storage full?
        bool full() const noexcept
        {
            return size() == capacity();
        }

        //! Max size that can fit within container
        static constexpr size_type max_size() noexcept
        {
            return capacity();
        }

        //! Constructs a new element at the end of the storage
        //! in-place.
        //!
        //! Increases size of the storage by one.
        //! Always fails for empty storage.
        template <typename... Args, typename = enable_if_t<is_constructible_v<T, Args...>>>
        reference emplace_back(Args&&... args) noexcept(is_nothrow_constructible_v<T, Args...>)
        {
            AZSTD_CONTAINER_ASSERT(!full(), "emplace_back cannot be invoked on full storage");
            reference new_element = *(data() + size());
            AZStd::construct_at(&new_element, AZStd::forward<Args>(args)...);
            resize_no_construct(size() + 1);
            return new_element;
        }
        //! Removes the last element of the storage.
        //! Precondition: size is not empty
        void pop_back() noexcept(is_nothrow_destructible_v<value_type>)
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "pop_back cannot be invoked on empty storage");
            reference last_element = *(data() + size() - 1);
            AZStd::destroy_at(&last_element);
            resize_no_construct(size() - 1);
        }
        //! Changes the size of the storage without adding or
        //! removing elements (unsafe).
        //!
        //! Updates the size of the container while checking that the new size is less than capacity
        void resize_no_construct(size_t new_size) noexcept
        {
            AZSTD_CONTAINER_ASSERT(new_size <= capacity(), "New size cannot be larger than capacity");
            m_size = aznumeric_cast<size_type>(new_size);
        }

        //! Destructs elements in the range [begin, end).
        //! This does not modify the size of the storage
        //! Invokes the destuctor via the AZStd::destroy method
        template <typename InputIt, typename = enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        void unsafe_destroy(InputIt first, InputIt last) noexcept(is_nothrow_destructible_v<value_type>)
        {
            AZSTD_CONTAINER_ASSERT(first >= data() && first <= data() + size(), "begin iterator is not in range of storage");
            AZSTD_CONTAINER_ASSERT(last >= data() && last <= data() + size(), "last iterator is not in range of storage");
            AZStd::destroy(first, last);
        }

        //! Destructs all elements of the storage.
        //! This does not modify the size of the storage
        //! Invokes the destuctor via the Interna;::destory::range method
        void unsafe_destroy_all() noexcept(is_nothrow_destructible_v<value_type>)
        {
            unsafe_destroy(data(), data() + size());
        }

        //! Storage for T element - It uses aligned storage to avoid default construction
        //! of all Capacity elements
        AZStd::aligned_storage_for_t<T> m_data[Capacity];
        size_type m_size{}; //! Storage off current size of fixed vector storage
    };

    template<class T, size_t Capacity>
    using fixed_vector_storage = conditional_t<Capacity == 0,
        Internal::fixed_zero_size_storage<T>,
        conditional_t<is_trivial_v<T>, Internal::fixed_trivial_storage<T, Capacity>,
        Internal::fixed_non_trivial_storage<T, Capacity>
        >>;
}

namespace AZStd
{
    /**
     * This is the fixed capacity version of the \ref AZStd::vector. All of the functionality is the same
     * except we do not have allocator and never allocate memory. We can not change the capacity, it
     * is set at compile time. leak_and_reset function just does the reset part, no destructor is called.
     *
     * \note Although big parts of the code is the same with vector, we do have separate implementation.
     * This class is made to be constexpr friendly - That being said it requires C++20 support in order
     * to allow constexpr in methods which invokes placement new and non-trivial destructors
     * Check the fixed_vector \ref AZStdExamples.
     */
    template< class T, AZStd::size_t Capacity>
    class fixed_vector
        : Internal::fixed_vector_storage<T, Capacity>
    {
        using base_type = Internal::fixed_vector_storage<T, Capacity>;
    public:
        using value_type = typename base_type::value_type;
        using difference_type = typename base_type::difference_type;
        using pointer = typename base_type::pointer;
        using const_pointer = typename base_type::const_pointer;
        using reference = typename base_type::reference;
        using const_reference = typename base_type::const_reference;

        using iterator = typename base_type::pointer;
        using const_iterator = typename base_type::const_pointer;
        // Different from fixed_vector_storage containers as algorithms expect size_t to be the size_type
        // for a container
        using size_type = size_t;
        using reverse_iterator = AZStd::reverse_iterator<iterator>;
        using const_reverse_iterator = AZStd::reverse_iterator<const_iterator>;

        // AZStd extension.
        using node_type = value_type;

        //////////////////////////////////////////////////////////////////////////
        // 23.2.4.1 construct/copy/destroy
        fixed_vector() = default;

        explicit fixed_vector(size_type numElements, const_reference value = value_type())
        {
            resize_no_construct(numElements);
            AZStd::uninitialized_fill_n(data(), numElements, value);
        }

        template <class InputIt, typename = AZStd::enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        fixed_vector(InputIt first, InputIt last)
        {
            resize_no_construct(AZStd::distance(first, last));
            AZStd::uninitialized_copy(first, last, data());
        }


        fixed_vector(const fixed_vector& rhs)
        {
            resize_no_construct(rhs.size());
            AZStd::uninitialized_copy(rhs.data(), rhs.data() + rhs.size(), data());
        }

        // The move constructor is linear in assignment operations being O(n)
        // It performs an AZStd::move on each of the fixed_vector elements instead
        // of swapping pointers to the allocted memory address
        // as it is unable to perform that operations due to the storage being baked into the container
        fixed_vector(fixed_vector&& rhs)
        {
            resize_no_construct(rhs.size());
            AZStd::uninitialized_move(rhs.data(), rhs.data() + rhs.size(), data());
            // Clear the elements in the moved from vector
            rhs.clear();
        }

        // Extension csontructor for copying other vector like container types
        // into a fixed_vector given that the type in question isn't the same type as this fixed_vector type
        template <typename VectorContainer, typename = AZStd::enable_if_t<!AZStd::is_same_v<VectorContainer, fixed_vector>
            && !AZStd::is_convertible_v<VectorContainer, size_type>>>
        fixed_vector(VectorContainer&& rhs)
        {
            constexpr bool is_const_or_lvalue_reference = AZStd::is_lvalue_reference_v<VectorContainer>
                || AZStd::is_const_v<VectorContainer>;

            // Update the container size to the source vector like container
            resize_no_construct(rhs.size());
            if constexpr (is_const_or_lvalue_reference)
            {
                AZStd::uninitialized_copy(rhs.data(), rhs.data() + rhs.size(), data());
            }
            else
            {
                AZStd::uninitialized_move(rhs.data(), rhs.data() + rhs.size(), data());
                // Clear the elements in the moved from vector
                rhs.clear();
            }
        }

        fixed_vector(AZStd::initializer_list<value_type> ilist)
            : base_type(ilist)
        {
        }

        fixed_vector& operator=(const fixed_vector& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

            // Explicitly invoke the template copy asignment function
            return assign_helper(rhs);
        }

        fixed_vector& operator=(fixed_vector&& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

            // Explicitly invoke the template move asignment function
            return assign_helper(AZStd::move(rhs));
        }

        template <typename VectorContainer>
        AZStd::enable_if_t<!AZStd::is_same_v<AZStd::remove_cvref_t<VectorContainer>, fixed_vector>, fixed_vector>& operator=(VectorContainer&& rhs)
        {
            return assign_helper(AZStd::forward<VectorContainer>(rhs));
        }

        iterator begin() { return iterator(data()); }
        const_iterator begin() const { return const_iterator(data()); }
        const_iterator cbegin() const { return const_iterator(data()); }
        iterator end() { return iterator(data() + size()); }
        const_iterator end() const { return const_iterator(data() + size()); }
        const_iterator cend() const { return const_iterator(data() + size()); }
        reverse_iterator rbegin() { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        reverse_iterator rend() { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

        // bring in fixed_vector_storage functions into scope
        using base_type::data;
        using base_type::empty;
        using base_type::full;
        using base_type::emplace_back;
        using base_type::pop_back;
        // extension method
        using base_type::resize_no_construct;

        size_type size() const noexcept
        {
            return base_type::size();
        }
        static constexpr size_type capacity() noexcept
        {
            return base_type::capacity();
        }
        static constexpr size_type max_size() noexcept
        {
            return base_type::max_size();
        }

        void resize(size_type newSize)
        {
            return resize(newSize, value_type{});
        }

        void resize(size_type newSize, const_reference value)
        {
            size_type dataSize = size();
            if (dataSize < newSize)
            {
                insert(begin() + dataSize, newSize - dataSize, value);
            }
            else if (newSize < dataSize)
            {
                erase(begin() + newSize, data() + dataSize);
            }
        }

        // Removes unused capacity - For fixed_vector this only asserts
        // that the supplied capacity is not longer than the fixed_vector capacity
        void reserve(size_type newCapacity)
        {
            // No-op - Implemented to provide consistent std::vector
            AZSTD_CONTAINER_ASSERT(newCapacity <= capacity(),
                "While fixed_vector capacity cannot be changed,"
                " attempting to supply a capacity larger than %zu is against the preconditions of this container", capacity());
        }

        // Removes unused capacity - For fixed_vector this does nothing
        void shrink_to_fit()
        {
            // No-op - Implemented to provide consistent std::vector
        }

        reference at(size_type position)
        {
            AZSTD_CONTAINER_ASSERT(position < size(), "AZStd::fixed_vector<>::at - position is out of range");
            return *(data() + position);
        }
        const_reference at(size_type position) const
        {
            AZSTD_CONTAINER_ASSERT(position < size(), "AZStd::fixed_vector<>::at - position is out of range");
            return *(data() + position);
        }

        reference operator[](size_type position)
        {
            AZSTD_CONTAINER_ASSERT(position < size(), "AZStd::fixed_vector<>::at - position is out of range");
            return *(data() + position);
        }
        const_reference operator[](size_type position) const
        {
            AZSTD_CONTAINER_ASSERT(position < size(), "AZStd::fixed_vector<>::at - position is out of range");
            return *(data() + position);
        }

        reference front()
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "AZStd::fixed_vector<>::front - container is empty!");
            return *data();
        }
        const_reference front() const
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "AZStd::fixed_vector<>::front - container is empty!");
            return *data();
        }
        reference back()
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "AZStd::fixed_vector<>::back - container is empty!");
            return *(data() + size() - 1);
        }
        const_reference back() const
        {
            AZSTD_CONTAINER_ASSERT(!empty(), "AZStd::fixed_vector<>::back - container is empty!");
            return *(data() + size() - 1);
        }

        void push_back(const_reference value)
        {
            emplace_back(value);
        }

        void assign(size_type numElements, const_reference value)
        {
            clear();
            insert(end(), numElements, value);
        }

        template <class InputIt, typename = AZStd::enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        void assign(InputIt first, InputIt last)
        {
            clear();
            insert(end(), first, last);
        }

        void assign(AZStd::initializer_list<value_type> ilist)
        {
            assign(ilist.begin(), ilist.end());
        }

        template <typename... Args, typename = AZStd::enable_if_t<is_constructible_v<T, Args...>>>
        iterator emplace(const_iterator insertPos, Args&&... args)
        {
            AZSTD_CONTAINER_ASSERT(!full(), "Cannot emplace on a full fixed_vector");
            AZSTD_CONTAINER_ASSERT(insertPos >= cbegin() && insertPos <= cend(), "insert position must be in range of container");
            pointer dataEnd = data() + size();
            pointer insertPosPtr = data() + AZStd::distance(cbegin(), insertPos);

            if (insertPosPtr == dataEnd)
            {
                reference newElement = emplace_back(AZStd::forward<Args>(args)...);
                return &newElement;
            }

            AZStd::uninitialized_move(insertPosPtr, dataEnd, insertPosPtr + 1);
            AZStd::construct_at(insertPosPtr, AZStd::forward<Args>(args)...);
            return iterator(insertPosPtr);
        }
        iterator insert(const_iterator insertPos, const_reference value)
        {
            AZSTD_CONTAINER_ASSERT(insertPos >= cbegin() && insertPos <= cend(), "insert position must be in range of container");
            return emplace(insertPos, value);
        }
        iterator insert(const_iterator insertPos, value_type&& value)
        {
            AZSTD_CONTAINER_ASSERT(insertPos >= cbegin() && insertPos <= cend(), "insert position must be in range of container");
            return emplace(insertPos, AZStd::move(value));
        }

        void insert(const_iterator insertPos, size_type numElements, const_reference value)
        {
            if (numElements == 0)
            {
                return;
            }

            AZSTD_CONTAINER_ASSERT(insertPos >= cbegin() && insertPos <= cend(), "insert position must be in range of container");
            AZSTD_CONTAINER_ASSERT(Capacity >= (size() + numElements), "AZStd::fixed_vector::insert - capacity is reached!");
            pointer dataStart = data();
            pointer dataEnd = data() + size();
            pointer insertPosPtr = data() + AZStd::distance(cbegin(), insertPos);

            const_pointer valuePtr = AZStd::addressof(value);
            const bool valueOverlapsInputRange = valuePtr >= insertPosPtr && valuePtr < dataEnd;

            size_type numInitializedToFill = AZStd::distance(insertPosPtr, dataEnd);
            if (numInitializedToFill < numElements)
            {
                // Number of elements we can just set not init needed.

                // Move the elements after insert position.
                pointer newLast = AZStd::uninitialized_move(insertPosPtr, dataEnd, insertPosPtr + numElements);

                // Add new elements to uninitialized elements.
                AZStd::uninitialized_fill_n(dataEnd, numElements - numInitializedToFill, valueOverlapsInputRange ? value_type(value) : value);

                // Add new data
                AZStd::fill_n(insertPosPtr, numInitializedToFill, valueOverlapsInputRange ? value_type(value) : value);

                // Update the size of the fixed_vector_storage
                resize_no_construct(AZStd::distance(dataStart, newLast));
            }
            else
            {
                pointer nonOverlap = dataEnd - numElements;

                // first copy the data that will not overlap.
                pointer newLast = AZStd::uninitialized_move(nonOverlap, dataEnd, dataEnd);

                // move the area with overlapping
                AZStd::move_backward(insertPosPtr, nonOverlap, dataEnd);

                // add new elements
                AZStd::fill_n(insertPosPtr, numElements, valueOverlapsInputRange ? value_type(value) : value);

                // Update the size of the fixed_vector_storage
                resize_no_construct(AZStd::distance(dataStart, newLast));
            }
        }

        template<class InputIt, typename = AZStd::enable_if_t<Internal::is_input_iterator_v<InputIt>>>
        void insert(const_iterator insertPos, InputIt first, InputIt last)
        {
            // specialize for iterator categories.
            AZSTD_CONTAINER_ASSERT(insertPos >= cbegin() && insertPos <= cend(), "insert position must be in range of container");
            insert_iter(insertPos, first, last, typename iterator_traits<InputIt>::iterator_category());
        };

        void insert(const_iterator insertPos, AZStd::initializer_list<value_type> ilist)
        {
            insert(insertPos, ilist.begin(), ilist.end());
        }

        iterator erase(const_iterator elementIter)
        {
            return erase(elementIter, elementIter + 1);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            AZSTD_CONTAINER_ASSERT(first >= cbegin() && last <= cend(), "erase iterator must be inside the range of fixed_vector container");
            iterator dataStart = begin();
            iterator dataEnd = end();
            iterator firstPtr = begin() + AZStd::distance(cbegin(), first);
            iterator lastPtr = begin() + AZStd::distance(cbegin(), last);
            size_type offset = AZStd::distance(dataStart, firstPtr);
            // unless we have 1 elements we have memory overlapping, so we need to use move.
            iterator newLast = AZStd::move(lastPtr, dataEnd, firstPtr);
            base_type::unsafe_destroy(newLast, dataEnd);
            resize_no_construct(AZStd::distance(dataStart, newLast));

            return dataStart + offset;
        }

        void clear()
        {
            base_type::unsafe_destroy_all();
            resize_no_construct(0);
        }
        void swap(fixed_vector& rhs)
        {
            // Fixed containers cannot swap pointers, they need to do full copies.
            // The strategy is to extend the smaller fixed_vector to be the size
            // of the larger fixed_vector and then swap in place

            // Fixed the larger and smaller of the containers
            fixed_vector& largerContainer = size() > rhs.size() ? *this : rhs;
            fixed_vector& smallerContainer = size() > rhs.size() ? rhs : *this;

            // Extend the smaller container with the elements in the largerContainer
            size_type smallerSize = smallerContainer.size();
            smallerContainer.insert(smallerContainer.end(), largerContainer.begin() + smallerSize, largerContainer.end());

            // Perform an inplace swap of both containers up to the size of the smaller container
            auto first1 = smallerContainer.begin();
            auto first2 = largerContainer.begin();
            auto last1 = smallerContainer.end();
            for (; first1 != last1; ++first1, ++first2)
            {
                auto tmp = AZStd::move(*first1);
                *first1 = AZStd::move(*first2);
                *first2 = AZStd::move(tmp);
            }

            // Destruct the elements in the larger container back down to the size of the smaller container
            largerContainer.erase(largerContainer.begin() + smallerSize, largerContainer.end());
        }

        // Validate container status.
        bool validate() const
        {
            return size() <= max_size();
        }
        // Validate iterator.
        int validate_iterator(const_iterator iter) const
        {
            const_pointer start = data();
            const_pointer end = data() + size();
            const_pointer iterPtr = iter;
            if (iterPtr < start || iterPtr > end)
            {
                return isf_none;
            }
            else if (iterPtr == end)
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        // pushes back an empty without a provided instance.
        void push_back()
        {
            emplace_back();
        }

        void leak_and_reset()
        {
            resize_no_construct(0);
        }

    private:
        template <typename VectorContainer>
        fixed_vector& assign_helper(VectorContainer&& rhs)
        {
            constexpr bool is_const_or_lvalue_reference = AZStd::is_lvalue_reference_v<VectorContainer>
                || AZStd::is_const_v<VectorContainer>;

            auto rhsStart = rhs.data();
            size_type newSize = rhs.size();
            pointer dataStart = data();
            size_type dataSize = size();

            if (newSize == 0)
            {
                clear(); // New size is zero so empty out the container 
                return *this;
            }

            // resize_no_construct updates the size of the fixed_vector_storage
            // and asserts if the new size is greater than the capacity
            resize_no_construct(newSize);

            if (dataSize >= newSize)
            {
                // We have capacity to hold the new data.
                iterator newLast{};
                if constexpr (is_const_or_lvalue_reference)
                {
                    newLast = AZStd::copy(rhsStart, rhsStart + newSize, dataStart);
                }
                else
                {
                    newLast = AZStd::move(rhsStart, rhsStart + newSize, dataStart);
                }

                // Destroy the rest.
                AZStd::destroy(newLast, dataStart + dataSize);
            }
            else
            {
                iterator newLast{};
                if constexpr (is_const_or_lvalue_reference)
                {
                    newLast = AZStd::copy(rhsStart, rhsStart + dataSize, dataStart);
                    AZStd::uninitialized_copy(rhsStart + dataSize, rhsStart + newSize, newLast);
                }
                else
                {
                    newLast = AZStd::move(rhsStart, rhsStart + dataSize, dataStart);
                    AZStd::uninitialized_move(rhsStart + dataSize, rhsStart + newSize, newLast);
                }
            }

            if constexpr (!is_const_or_lvalue_reference)
            {
                // Clear content from the moved container 
                rhs.clear();
            }

            return *this;
        }

        template<class Iterator>
        void insert_iter(const_iterator insertPos, Iterator first, Iterator last, const forward_iterator_tag&)
        {
            size_type numElements = AZStd::distance(first, last);
            if (numElements == 0)
            {
                return;
            }

            pointer insertPosPtr = data() + AZStd::distance(cbegin(), insertPos);

            AZSTD_CONTAINER_ASSERT(Capacity >= size() + numElements, "AZStd::fixed_vector::insert_iter - capacity is reached!");

            pointer dataStart = data();
            pointer dataEnd = data() + size();
            // Number of elements we can assign.
            size_type numInitializedToFill = AZStd::distance(insertPosPtr, dataEnd);
            if (numInitializedToFill < numElements)
            {
                // Copy the elements after insert position.
                AZStd::uninitialized_move(insertPosPtr, dataEnd, insertPosPtr + numElements);
                // get last iterator to use move assignment operator
                Iterator lastToAssign = AZStd::next(first, numInitializedToFill);

                // assign new data - The data up to previous dataEnd has already been constructed
                // so placement new should not be used for them
                insertPosPtr = AZStd::copy(first, lastToAssign, insertPosPtr);
                // Add new elements to uninitialized elements.
                iterator newDataEnd = AZStd::uninitialized_copy(lastToAssign, last, insertPosPtr);

                // Update the fixed storage size
                resize_no_construct(AZStd::distance(dataStart, newDataEnd));
            }
            else
            {
                // We need to copy data with care, it is overlapping.

                // first copy the data that will not overlap.
                pointer nonOverlap = dataEnd - numElements;
                iterator  newLast = AZStd::uninitialized_move(nonOverlap, dataEnd, dataEnd);

                // copy the memory backwards while performing AZStd::move on the existing elements the area with overlapping memory
                AZStd::move_backward(insertPosPtr, nonOverlap, dataEnd);

                // add new elements
                AZStd::move(first, last, insertPosPtr);

                resize_no_construct(AZStd::distance(dataStart, newLast));
            }
        }

        template<class Iterator>
        void insert_iter(const_iterator insertPos, Iterator first, Iterator last, const input_iterator_tag&)
        {
            iterator dataStart = data();
            size_type offset = AZStd::distance(dataStart, insertPos);

            for (Iterator iter{ first }; iter != last; ++iter, ++offset)
            {
                insert(dataStart + offset, *iter);
            }
        }
    };

    //! Deduction Guide for AZStd::fixed_vector
    template<class T, class... U>
    fixed_vector(T, U...) -> fixed_vector<T, 1 + sizeof...(U)>;

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator==(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return AZStd::equal(a.begin(), a.end(), b.begin(), b.end());
    }

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator!=(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return !operator==(a, b);
    }

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator<(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return AZStd::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
    }

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator<=(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return !operator<(b, a);
    }

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator>(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return operator<(b, a);
    }

    template< class T, AZStd::size_t Capacity1, AZStd::size_t Capacity2>
    constexpr bool operator>=(const fixed_vector<T, Capacity1>& a, const fixed_vector<T, Capacity2>& b)
    {
        return !operator<(a, b);
    }

    // C++20 erase free functions
    template<class T, size_t Capacity, class U>
    constexpr decltype(auto) erase(fixed_vector<T, Capacity>& container, const U& value)
    {
        auto iter = AZStd::remove(container.begin(), container.end(), value);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }
    template<class T, size_t Capacity, class Predicate>
    constexpr decltype(auto) erase_if(fixed_vector<T, Capacity>& container, Predicate predicate)
    {
        auto iter = AZStd::remove_if(container.begin(), container.end(), predicate);
        auto removedCount = AZStd::distance(iter, container.end());
        container.erase(iter, container.end());
        return removedCount;
    }
}
