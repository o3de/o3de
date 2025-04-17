/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/createdestroy.h>

// Until C++20 std::is_constant_evaluated type trait is supported
// The following macro is used for Assert condition checking within a constexpr function
// It works at compile time by evaluating the assert condition before
// evaluating whether the code within the if block invokes a non-constexpr function.
// Because the AZ_Assert macro invokes the "AZ::Debug::Trace::Instance().GetAssertNoOp()" function
// before it checks the condition, it fails due to that function not being a constexpr.
//
// By evaluating the assert condition first, if the assert condition fails, the compiler would evaluate
// the inner block which invokes a non-constexpr function causing a hard error, otherwise if the assert
// condition succeeds the inner block isn't evaluated and the constexpr function is able to be compiled
#define AZSTD_CONTAINER_COMPILETIME_ASSERT(expression, ...) \
    if (!(expression)) \
    { \
        AZ_Assert(expression, __VA_ARGS__); \
    }

namespace AZStd
{
    /**
     * Array container is complaint with \ref CTR1 (6.2.2)
     * It's just a static array container. You are allowed to initialize it like C array:
     * array<int,5> myData = {{1,2,3,4,5}};
     * Check the array \ref AZStdExamples.
     */
    template<class T, size_t N>
    class array
    {
        typedef array<T, N> this_type;
    public:
        //#pragma region Type definitions
        typedef T* pointer;
        typedef const T* const_pointer;

        typedef T& reference;
        typedef const T& const_reference;
        typedef ptrdiff_t difference_type;
        typedef size_t size_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;

        typedef AZStd::reverse_iterator<iterator> reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T value_type;

        // AZSTD extension.
        typedef value_type node_type;
        //#pragma endregion

        enum
        {
            array_size = N
        };

        constexpr iterator begin() { return m_elements; }
        constexpr const_iterator begin() const { return m_elements; }
        constexpr iterator end() { return m_elements + N; }
        constexpr const_iterator end() const { return m_elements + N; }

        constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }
        constexpr const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
        constexpr reverse_iterator rend() { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

        constexpr const_iterator cbegin() const { return m_elements; }
        constexpr const_iterator cend() const { return m_elements + N; }
        constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

        constexpr reference front() { return m_elements[0]; }
        constexpr const_reference front() const { return m_elements[0]; }
        constexpr reference back() { return m_elements[N - 1]; }
        constexpr const_reference back() const { return m_elements[N - 1]; }

        constexpr reference operator[](size_type i)
        {
            AZSTD_CONTAINER_COMPILETIME_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        constexpr const_reference operator[](size_type i) const
        {
            AZSTD_CONTAINER_COMPILETIME_ASSERT(i < N, "out of range");
            return m_elements[i];
        }

        constexpr reference at(size_type i) { AZSTD_CONTAINER_COMPILETIME_ASSERT(i < N, "out of range");  return m_elements[i]; }
        constexpr const_reference at(size_type i) const { AZSTD_CONTAINER_COMPILETIME_ASSERT(i < N, "out of range");  return m_elements[i]; }

        // size is constant
        constexpr static size_type size() { return N; }
        constexpr static bool empty() { return false; }
        constexpr static size_type max_size() { return N; }


        // swap
        void swap(this_type& other) { AZStd::swap_ranges(m_elements, pointer(m_elements + N), other.m_elements); }

        // direct access to data (read-only)
        constexpr const T* data() const { return m_elements; }
        constexpr T* data() { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        constexpr array<T, N>& operator = (const array<T2, N>& rhs)
        {
            AZStd::Internal::copy(rhs.m_elements, rhs.m_elements + N, m_elements);
            return *this;
        }

        constexpr void fill(const T& value)
        {
            AZStd::fill_n(m_elements, N, value);
        }

        constexpr bool validate() const { return true; }
        // Validate iterator.
        constexpr int validate_iterator(const iterator& iter) const
        {
            if (iter < m_elements || iter > (m_elements + N))
            {
                return isf_none;
            }
            else if (iter == (m_elements + N))
            {
                return isf_valid;
            }

            return isf_valid | isf_can_dereference;
        }

        //private:
        T m_elements[N];
    };

    template<class T>
    class array<T, 0U>
    {
        typedef array<T, 0> this_type;
    public:
        //#pragma region Type definitions
        typedef T* pointer;
        typedef const T* const_pointer;

        typedef T& reference;
        typedef const T& const_reference;
        typedef ptrdiff_t difference_type;
        typedef size_t size_type;
        typedef pointer iterator;
        typedef const_pointer const_iterator;

        typedef AZStd::reverse_iterator<iterator> reverse_iterator;
        typedef AZStd::reverse_iterator<const_iterator> const_reverse_iterator;
        typedef T value_type;

        // AZSTD extension.
        typedef value_type node_type;
        //#pragma endregion

        enum
        {
            array_size = 0
        };

        constexpr iterator begin() { return m_elements; }
        constexpr const_iterator begin() const { return m_elements; }
        constexpr iterator end() { return m_elements; }
        constexpr const_iterator end() const { return m_elements; }

        constexpr reverse_iterator rbegin() { return reverse_iterator(end()); }
        constexpr const_reverse_iterator  rbegin() const { return const_reverse_iterator(end()); }
        constexpr reverse_iterator rend() { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator rend() const { return const_reverse_iterator(end()); }

        constexpr const_iterator cbegin() const { return m_elements; }
        constexpr const_iterator cend() const { return m_elements; }
        constexpr const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crend() const { return const_reverse_iterator(end()); }

        constexpr reference front() { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        constexpr const_reference front() const { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        constexpr reference back() { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }
        constexpr const_reference back() const { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0"); return m_elements[0]; }

        constexpr reference operator[](size_type)
        {
            AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0");
            return m_elements[0];
        }

        constexpr const_reference operator[](size_type) const
        {
            AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0");
            return m_elements[0];
        }

        constexpr reference at(size_type) { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }
        constexpr const_reference at(size_type) const { AZSTD_CONTAINER_COMPILETIME_ASSERT(false, "out of range. Cannot access elements in an array of size 0");  return m_elements[0]; }

        // size is constant
        constexpr static size_type size() { return 0; }
        constexpr static bool empty() { return true; }
        constexpr static size_type max_size() { return 0; }


        // swap
        constexpr void swap(this_type&) {}

        // direct access to data (read-only)
        constexpr const T* data() const { return m_elements; }
        constexpr T* data() { return m_elements; }

        // assignment with type conversion
        template <typename T2>
        constexpr array<T, 0>& operator = (const array<T2, 0>& rhs) { return *this; }

        constexpr void fill(const T&) {}

        constexpr bool validate() const { return true; }
        // Validate iterator.
        constexpr int validate_iterator(const iterator& iter) const { return iter == m_elements ? isf_valid : isf_none; }

        T m_elements[1]; // The minimum size of a class in C++ is 1 byte, so use an array of size 1 as the sentinel value
    };

    // Deduction Guide for AZStd::array
    template<class T, class... U>
    array(T, U...)->array<T, 1 + sizeof...(U)>;

    //#pragma region Vector equality/inequality
    template<class T, size_t N>
    bool operator==(const array<T, N>& a, const array<T, N>& b)
    {
        return AZStd::equal(a.begin(), a.end(), b.begin(), b.end());
    }

    template<class T, size_t N>
    bool operator!=(const array<T, N>& a, const array<T, N>& b)
    {
        return !(a == b);
    }
    //#pragma endregion

    // std::to_array
    namespace Internal
    {
        template<class T, size_t N, size_t... Is>
        constexpr array<remove_cv_t<T>, sizeof...(Is)> to_array(T (&arr)[N], index_sequence<Is...>)
        {
            return { {arr[Is] ...} };
        }
        template<class T, size_t N, size_t... Is>
        constexpr array<remove_cv_t<T>, sizeof...(Is)> to_array(T (&&arr)[N], index_sequence<Is...>)
        {
            return { {AZStd::move(arr[Is]) ...} };
        }
    }
    template<class T, size_t N>
    constexpr array<remove_cv_t<T>, N> to_array(T (&arr)[N])
    {
        return Internal::to_array(arr, make_index_sequence<N>{});
    }

    template<class T, size_t N>
    constexpr array<remove_cv_t<T>, N> to_array(T (&&arr)[N])
    {
        return Internal::to_array(AZStd::move(arr), make_index_sequence<N>{});
    }
}

#undef AZSTD_CONTAINER_COMPILETIME_ASSERT
