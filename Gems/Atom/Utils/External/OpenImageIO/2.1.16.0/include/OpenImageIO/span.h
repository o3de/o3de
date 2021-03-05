// Copyright 2008-presennt Contributors to the OpenImageIO project.
//  https://github.com/OpenImageIO/oiio
// BSD 3-clause license:
//  https://github.com/OpenImageIO/oiio/blob/master/LICENSE

// clang-format off

#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>

OIIO_NAMESPACE_BEGIN


constexpr ptrdiff_t dynamic_extent = -1;


/// span<T> is a non-owning, non-copying, non-allocating reference to a
/// contiguous array of T objects known length. A 'span` encapsulates both a
/// pointer and a length, and thus is a safer way of passing pointers around
/// (because the function called knows how long the array is). A function
/// that might ordinarily take a `T*` and a length could instead just take a
/// `span<T>`.
///
/// `A span<T>` is mutable (the values in the array may be modified).  A
/// non-mutable (i.e., read-only) reference would be `span<const T>`. Thus,
/// a function that might ordinarily take a `const T*` and a length could
/// instead take a `span<const T>`.
///
/// For convenience, we also define `cspan<T>` as equivalent to
/// `span<const T>`.
///
/// A `span` may be initialized explicitly from a pointer and length, by
/// initializing with a `std::vector<T>`, or by initalizing with a constant
/// (treated as an array of length 1). For all of these cases, no extra
/// allocations are performed, and no extra copies of the array contents are
/// made.
///
/// Important caveat: The `span` merely refers to items owned by another
/// array, so the `span` should not be used beyond the lifetime of the
/// array it refers to. Thus, `span` is great for parameter passing, but
/// it's not a good idea to use a `span` to store values in a data
/// structure (unless you are really sure you know what you're doing).
///

template <typename T, ptrdiff_t Extent = dynamic_extent>
class span {
    static_assert (std::is_array<T>::value == false, "can't have span of an array");
public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using index_type = ptrdiff_t;
    using difference_type = ptrdiff_t;
    using size_type = ptrdiff_t;
    using pointer = element_type*;
    using reference = element_type&;
    using iterator = element_type*;
    using const_iterator = const element_type*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    static constexpr index_type extent = Extent;

    /// Default constructor -- the span points to nothing.
    constexpr span () noexcept { }

    /// Copy constructor (copies the span pointer and length, NOT the data).
    template<class U, ptrdiff_t N>
    constexpr span (const span<U,N> &copy) noexcept
        : m_data(copy.data()), m_size(copy.size()) { }
    /// Copy constructor (copies the span pointer and length, NOT the data).
    constexpr span (const span &copy) noexcept = default;

    /// Construct from T* and length.
    constexpr span (pointer data, index_type size) noexcept
        : m_data(data), m_size(size) { }

    /// Construct from begin and end pointers.
    constexpr span (pointer b, pointer e) noexcept
        : m_data(b), m_size(e-b) { }

    /// Construct from a single T&.
    constexpr span (T &data) : m_data(&data), m_size(1) { }

    /// Construct from a fixed-length C array.  Template magic automatically
    /// finds the length from the declared type of the array.
    template<size_t N>
    constexpr span (T (&data)[N]) : m_data(data), m_size(N) { }

    /// Construct from std::vector<T>.
    template<class Allocator>
    constexpr span (std::vector<T, Allocator> &v)
        : m_data(v.size() ? &v[0] : nullptr), m_size(v.size()) {
    }

    /// Construct from `const std::vector<T>.` This turns
    /// `const std::vector<T>` into a `span<const T>` (the span isn't const,
    /// but the data it points to will be).
    template<class Allocator>
    span (const std::vector<value_type, Allocator> &v)
        : m_data(v.size() ? &v[0] : nullptr), m_size(v.size()) { }

    /// Construct from mutable element std::array
    template <size_t N>
    constexpr span (std::array<value_type, N> &arr)
        : m_data(arr.data()), m_size(N) {}

    /// Construct from read-only element std::array
    template <size_t N>
    constexpr span (const std::array<value_type, N>& arr)
        : m_data(arr.data()), m_size(N) {}

    /// Construct an span from an initializer_list.
    constexpr span (std::initializer_list<T> il)
        : span (il.begin(), il.size()) { }

    /// Assignment copies the pointer and length, not the data.
    span& operator= (const span &copy) {
        m_data = copy.data();
        m_size = copy.size();
        return *this;
    }

    /// Subview containing the first Count elements of the span.
    template<index_type Count>
    constexpr span<element_type, Count> first () const {
        return { m_data, Count };
    }
    /// Subview containing the last Count elements of the span.
    template<index_type Count>
    constexpr span<element_type, Count> last () const {
        return { m_data + m_size - Count, Count };
    }

    template<index_type Offset, index_type Count = dynamic_extent>
    constexpr span<element_type, Count> subspan () const {
        return { m_data + Offset, Count != dynamic_extent ? Count : (Extent != dynamic_extent ? Extent - Offset : m_size - Offset) };
    }

    constexpr span<element_type, dynamic_extent> first (index_type count) const {
        return { m_data, count };
    }

    constexpr span<element_type, dynamic_extent> last (index_type count) const {
        return { m_data + ( m_size - count ), count };
    }

    constexpr span<element_type, dynamic_extent>
    subspan (index_type offset, index_type count = dynamic_extent) const {
        return { m_data + offset, count == dynamic_extent ? m_size - offset : count };
    }

    constexpr index_type size() const noexcept { return m_size; }
    constexpr index_type size_bytes() const noexcept { return m_size*sizeof(T); }
    constexpr bool empty() const noexcept { return m_size == 0; }

    constexpr pointer data() const noexcept { return m_data; }

    constexpr reference operator[] (index_type idx) const { return m_data[idx]; }
    constexpr reference operator() (index_type idx) const { return m_data[idx]; }
    reference at (index_type idx) const {
        if (idx >= size())
            throw (std::out_of_range ("OpenImageIO::span::at"));
        return m_data[idx];
    }

    constexpr reference front() const noexcept { return m_data[0]; }
    constexpr reference back() const noexcept { return m_data[size()-1]; }

    constexpr iterator begin() const noexcept { return m_data; }
    constexpr iterator end() const noexcept { return m_data + m_size; }

    constexpr const_iterator cbegin() const noexcept { return m_data; }
    constexpr const_iterator cend() const noexcept { return m_data + m_size; }

    constexpr reverse_iterator rbegin() const noexcept { return m_data + m_size - 1; }
    constexpr reverse_iterator rend() const noexcept { return m_data - 1; }

    constexpr const_reverse_iterator crbegin() const noexcept { return m_data + m_size - 1; }
    constexpr const_reverse_iterator crend() const noexcept { return m_data - 1; }

private:
    pointer     m_data = nullptr;
    index_type  m_size = 0;
};



/// cspan<T> is a synonym for a non-mutable span<const T>.
template <typename T>
using cspan = span<const T>;



/// Compare all elements of two spans for equality
template <class T, ptrdiff_t X, class U, ptrdiff_t Y>
OIIO_CONSTEXPR14 bool operator== (span<T,X> l, span<U,Y> r) {
#if OIIO_CPLUSPLUS_VERSION >= 20
    return std::equal (l.begin(), l.end(), r.begin(), r.end());
#else
    auto lsize = l.size();
    bool same = (lsize == r.size());
    for (ptrdiff_t i = 0; same && i < lsize; ++i)
        same &= (l[i] == r[i]);
    return same;
#endif
}

/// Compare all elements of two spans for inequality
template <class T, ptrdiff_t X, class U, ptrdiff_t Y>
OIIO_CONSTEXPR14 bool operator!= (span<T,X> l, span<U,Y> r) {
    return !(l == r);
}



/// span_strided<T> : a non-owning, mutable reference to a contiguous
/// array with known length and optionally non-default strides through the
/// data.  An span_strided<T> is mutable (the values in the array may
/// be modified), whereas an span_strided<const T> is not mutable.
template <typename T, ptrdiff_t Extent = dynamic_extent>
class span_strided {
    static_assert (std::is_array<T>::value == false,
                   "can't have span_strided of an array");
public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using index_type = ptrdiff_t;
    using difference_type = ptrdiff_t;
    using stride_type = ptrdiff_t;
    using pointer = element_type*;
    using reference = element_type&;
    using iterator = element_type*;
    using const_iterator = const element_type*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    static constexpr index_type extent = Extent;

    /// Default ctr -- points to nothing
    constexpr span_strided () noexcept {}

    /// Copy constructor
    constexpr span_strided (const span_strided &copy)
        : m_data(copy.data()), m_size(copy.size()), m_stride(copy.stride()) {}

    /// Construct from T* and size, and optionally stride.
    constexpr span_strided (pointer data, index_type size, stride_type stride=1)
        : m_data(data), m_size(size), m_stride(stride) { }

    /// Construct from a single T&.
    constexpr span_strided (T &data) : span_strided(&data,1,1) { }

    /// Construct from a fixed-length C array.  Template magic automatically
    /// finds the length from the declared type of the array.
    template<size_t N>
    constexpr span_strided (T (&data)[N]) : span_strided(data,N,1) {}

    /// Construct from std::vector<T>.
    template<class Allocator>
    OIIO_CONSTEXPR14 span_strided (std::vector<T, Allocator> &v)
        : span_strided(v.size() ? &v[0] : nullptr, v.size(), 1) {}

    /// Construct from const std::vector<T>. This turns const std::vector<T>
    /// into an span_strided<const T> (the span_strided isn't
    /// const, but the data it points to will be).
    template<class Allocator>
    constexpr span_strided (const std::vector<value_type, Allocator> &v)
        : span_strided(v.size() ? &v[0] : nullptr, v.size(), 1) {}

    /// Construct an span from an initializer_list.
    constexpr span_strided (std::initializer_list<T> il)
        : span_strided (il.begin(), il.size()) { }

    /// Initialize from an span (stride will be 1).
    constexpr span_strided (span<T> av)
        : span_strided(av.data(), av.size(), 1) { }

    // assignments
    span_strided& operator= (const span_strided &copy) {
        m_data   = copy.data();
        m_size   = copy.size();
        m_stride = copy.stride();
        return *this;
    }

    constexpr index_type size() const noexcept { return m_size; }
    constexpr stride_type stride() const noexcept { return m_stride; }

    constexpr reference operator[] (index_type idx) const {
        return m_data[m_stride*idx];
    }
    constexpr reference operator() (index_type idx) const {
        return m_data[m_stride*idx];
    }
    reference at (index_type idx) const {
        if (idx >= size())
            throw (std::out_of_range ("OpenImageIO::span_strided::at"));
        return m_data[m_stride*idx];
    }
    constexpr reference front() const noexcept { return m_data[0]; }
    constexpr reference back() const noexcept { return (*this)[size()-1]; }
    constexpr pointer data() const noexcept { return m_data; }

private:
    pointer       m_data   = nullptr;
    index_type    m_size   = 0;
    stride_type   m_stride = 1;
};



/// cspan_strided<T> is a synonym for a non-mutable span_strided<const T>.
template <typename T>
using cspan_strided = span_strided<const T>;



/// Compare all elements of two spans for equality
template <class T, ptrdiff_t X, class U, ptrdiff_t Y>
OIIO_CONSTEXPR14 bool operator== (span_strided<T,X> l, span_strided<U,Y> r) {
    auto lsize = l.size();
    if (lsize != r.size())
        return false;
    for (ptrdiff_t i = 0; i < lsize; ++i)
        if (l[i] != r[i])
            return false;
    return true;
}

/// Compare all elements of two spans for inequality
template <class T, ptrdiff_t X, class U, ptrdiff_t Y>
OIIO_CONSTEXPR14 bool operator!= (span_strided<T,X> l, span_strided<U,Y> r) {
    return !(l == r);
}


OIIO_NAMESPACE_END
