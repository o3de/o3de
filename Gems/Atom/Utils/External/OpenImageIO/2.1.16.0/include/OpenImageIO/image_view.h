// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <stdexcept>
#include <vector>

// We're including stdint.h to get int64_t and INT64_MIN. But on some
// platforms, stdint.h only defines them if __STDC_LIMIT_MACROS is defined,
// so we do so. But, oops, if user code included stdint.h before this file,
// and without defining the macro, it may have had ints one and only include
// and not seen the definitions we need, so at least try to make a helpful
// compile-time error in that case.
// And very old MSVC 9 versions don't even have stdint.h.
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef __int64 int64_t;
#else
#    ifndef __STDC_LIMIT_MACROS
#        define __STDC_LIMIT_MACROS /* needed for some defs in stdint.h */
#    endif
#    include <cstdint>
#    if !defined(INT64_MIN)
#        error You must define __STDC_LIMIT_MACROS prior to including stdint.h
#    endif
#endif

#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/strided_ptr.h>


OIIO_NAMESPACE_BEGIN


/// image_view : a non-owning reference to an image-like array (indexed by
/// x, y, z, and channel) with known dimensions and optionally non-default
/// strides (expressed in bytes) through the data.  An image_view<T> is
/// mutable (the values in the image may be modified), whereas an
/// image_view<const T> is not mutable.
template<typename T> class image_view {
public:
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef int64_t stride_t;
#ifdef INT64_MIN
    static const stride_t AutoStride = INT64_MIN;
#else
    // Some systems don't have INT64_MIN defined. Sheesh.
    static const stride_t AutoStride = (-9223372036854775807LL - 1);
#endif

    /// Default ctr -- points to nothing
    image_view() { init(); }

    /// Copy constructor
    image_view(const image_view& copy)
    {
        init(copy.m_data, copy.m_nchannels, copy.m_width, copy.m_height,
             copy.m_depth, copy.m_chanstride, copy.m_xstride, copy.m_ystride,
             copy.m_zstride);
    }

    /// Construct from T*, dimensions, and (possibly default) strides (in
    /// bytes).
    image_view(T* data, int nchannels, int width, int height, int depth = 1,
               stride_t chanstride = AutoStride, stride_t xstride = AutoStride,
               stride_t ystride = AutoStride, stride_t zstride = AutoStride)
    {
        init(data, nchannels, width, height, depth, chanstride, xstride,
             ystride, zstride);
    }

    /// assignments -- not a deep copy, just make this image_view
    /// point to the same data as the operand.
    image_view& operator=(const image_view& copy)
    {
        init(copy.m_data, copy.m_nchannels, copy.m_width, copy.m_height,
             copy.m_depth, copy.m_chanstride, copy.m_xstride, copy.m_ystride,
             copy.m_zstride);
        return *this;
    }

    /// iav(x,y,z)returns a strided_ptr<T,1> for the pixel (x,y,z).  The z
    /// can be omitted for 2D images.  Note than the resulting
    /// strided_ptr can then have individual channels accessed with
    /// operator[]. This particular strided pointer has stride multiplier
    /// 1, because this class uses bytes as strides, not sizeof(T).
    strided_ptr<T, 1> operator()(int x, int y, int z = 0)
    {
        return strided_ptr<T, 1>(getptr(0, x, y, z), m_chanstride);
    }

    int nchannels() const { return m_nchannels; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int depth() const { return m_depth; }

    stride_t chanstride() const { return m_chanstride; }
    stride_t xstride() const { return m_xstride; }
    stride_t ystride() const { return m_ystride; }
    stride_t zstride() const { return m_zstride; }

    const T* data() const { return m_data; }

    void clear() { init(); }

private:
    const T* m_data;
    int m_nchannels, m_width, m_height, m_depth;
    stride_t m_chanstride, m_xstride, m_ystride, m_zstride;

    void init(T* data, int nchannels, int width, int height, int depth = 1,
              stride_t chanstride = AutoStride, stride_t xstride = AutoStride,
              stride_t ystride = AutoStride, stride_t zstride = AutoStride)
    {
        m_data       = data;
        m_nchannels  = nchannels;
        m_width      = width;
        m_height     = height;
        m_depth      = depth;
        m_chanstride = chanstride != AutoStride ? chanstride : sizeof(T);
        m_xstride    = xstride != AutoStride ? xstride
                                          : m_nchannels * m_chanstride;
        m_ystride = ystride != AutoStride ? ystride : m_width * m_xstride;
        m_zstride = zstride != AutoStride ? zstride : m_height * m_ystride;
    }

    inline T* getptr(int c, int x, int y, int z = 0) const
    {
        return (T*)((char*)m_data + c * m_chanstride + x * m_xstride
                    + y * m_ystride + z * m_zstride);
    }
    inline T& get(int c, int x, int y, int z = 0) const
    {
        return *getptr(c, x, y, z);
    }
};


OIIO_NAMESPACE_END
