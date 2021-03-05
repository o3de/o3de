// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <OpenImageIO/oiioversion.h>

OIIO_NAMESPACE_BEGIN

/// VaryingRef is a templated class (on class T) that holds either a
/// pointer to a single T value, or an "array" of T values, each
/// separated by a certain number of bytes.  For those versed in the
/// lingo of SIMD shading, this encapsulates 'uniform' and 'varying'
/// references.
///
/// Suppose you have a computation 'kernel' that is performing an
/// operation while looping over several computation 'points.'  Each of
/// the several operands of the kernel may either be a 'uniform' value
/// (identical for each point), or 'varying' (having a potentially
/// different value for each point).
///
/// Here is a concrete example.  Suppose you have the following function:
///
///     void add (int n, float *a, float *b, float *result) {
///         for (int i = 0;  i < n;  ++i)
///             result[i] = a[i] + b[i];
///     }
///
/// But if the caller of this function has only a single b value (let's
/// say, you always want to add 3 to every a[i]), you would be forced
/// to replicate an entire array full of 3's in order to call the function.
///
/// Instead, we may wish to generalize the function so that each operand
/// may rever to EITHER a single value or an array of values, without
/// making the code more complicated.  We can do this with VaryingRef:
///
///     void add (int n, VaryingRef<float> a, VaryingRef<float> b,
///                      float *result) {
///         for (int i = 0;  i < n;  ++i)
///             result[i] = a[i] + b[i];
///     }
///
/// VaryingRef overloads operator [] to properly decode whether it is
/// uniform (point to the one value) or varying (index the right array
/// element).  It also overloads the increment operator ++ and the pointer
/// indirection operator '*', so you could also write the function
/// equivalently as:
///
///     void add (int n, VaryingRef<float> a, VaryingRef<float> b,
///                      float *result) {
///         for (int i = 0;  i < n;  ++i, ++a, ++b)   // note increments
///             result[i] = (*a) + (*b);
///     }
///
/// An example of calling this function would be:
///
///     float a[n];
///     float b;     // just 1 value
///     float result[n];
///     add (n, VaryingRef<float>(a,sizeof(a[0])),
///          VaryingRef<float>(b), result);
///
/// In this example, we're passing a truly varying 'a' (signified by
/// giving a step size from element to element), but a uniform 'b' (signified
/// by no step size, or a step size of zero).
///
/// There are Varying() and Uniform() templated functions that provide
/// a helpful shorthand:
///
///     add (n, Varying(a), Uniform(b), result);
///
/// Now let's take it a step further and fully optimize the 'add' function
/// for when both operands are uniform:
///
///     void add (int n, VaryingRef<float> a, VaryingRef<float> b,
///                      VaryingRef<float> result) {
///         if (a.is_uniform() && b.is_uniform()) {
///             float r = (*a) + (*b);
///             for (int i = 0;  i < n;  ++i)
///                 result[i] = r;
///         } else {
///             // One or both are varying
///             for (int i = 0;  i < n;  ++i, ++a, ++b)
///                 result[i] = (*a) + (*b);
///         }
///     }
///
/// This is the basis for handling uniform and varying values efficiently
/// inside a SIMD shading system.

template<class T> class VaryingRef {
public:
    VaryingRef() { init(0, 0); }

    /// Construct a VaryingRef either of a single value pointed to by ptr
    /// (if step == 0 or no step is provided), or of a varying set of
    /// values beginning with ptr and with successive values every 'step'
    /// bytes.
    VaryingRef(void* ptr_, int step_ = 0) { init((T*)ptr_, step_); }

    /// Construct a uniform VaryingRef from a single value.
    ///
    VaryingRef(T& ptr_) { init(&ptr_, 0); }

    /// Initialize this VaryingRef to either of a single value pointed
    /// to by ptr (if step == 0 or no step is provided), or of a varying
    /// set of values beginning with ptr and with successive values
    /// every 'step' bytes.
    void init(T* ptr_, int step_ = 0)
    {
        m_ptr  = ptr_;
        m_step = step_;
    }

    /// Initialize this VaryingRef to be uniform and point to a particular
    /// value reference.
    const VaryingRef& operator=(T& ptr_)
    {
        init(&ptr_);
        return *this;
    }

    /// Is this reference pointing nowhere?
    ///
    bool is_null() const { return (m_ptr == 0); }

    /// Cast to void* returns the pointer, but the real purpose is so
    /// you can use a VaryingRef as if it were a 'bool' value in a test.
    operator void*() const { return m_ptr; }

    /// Is this VaryingRef referring to a varying value, signified by
    /// having a nonzero step size between elements?
    bool is_varying() const { return (m_step != 0); }

    /// Is this VaryingRef referring to a uniform value, signified by
    /// having a step size of zero between elements?
    bool is_uniform() const { return (m_step == 0); }

    /// Pre-increment: If this VaryingRef is varying, increment its
    /// pointer to the next element in the series, but don't change
    /// anything if it's uniform.  In either case, return a reference to
    /// its new state.
    VaryingRef& operator++()
    {  // Prefix form ++i
        char* p = (char*)m_ptr;
        p += m_step;
        m_ptr = (T*)p;
        return *this;
    }
    /// Post-increment: If this VaryingRef is varying, increment its
    /// pointer to the next element in the series, but don't change
    /// anything if it's uniform.  No value is returned, so it's not
    /// legal to do 'bar = foo++' if foo and bar are VaryingRef's.
    void operator++(int)
    {  // Postfix form i++ : return nothing to avoid copy
        // VaryingRef<T> tmp = *this;
        char* p = (char*)m_ptr;
        p += m_step;
        m_ptr = (T*)p;
        // return tmp;
    }

    /// Pointer indirection will return the first value currently
    /// pointed to by this VaryingRef.
    T& operator*() const { return *m_ptr; }

    /// Array indexing operator will return a reference to the single
    /// element if *this is uniform, or to the i-th element of the
    /// series if *this is varying.
    T& operator[](int i) const { return *(T*)((char*)m_ptr + i * m_step); }

    /// Return the raw pointer underneath.
    ///
    T* ptr() const { return m_ptr; }

    /// Return the raw step underneath.
    ///
    int step() const { return m_step; }

private:
    T* m_ptr;    ///< Pointer to value
    int m_step;  ///< Distance between successive values -- in BYTES!
};



/// Helper function wraps a varying reference with default step size.
///
template<class T>
VaryingRef<T>
Varying(T* x)
{
    return VaryingRef<T>(x, sizeof(T));
}

/// Helper function wraps a uniform reference.
///
template<class T>
VaryingRef<T>
Uniform(T* x)
{
    return VaryingRef<T>(x, 0);
}

/// Helper function wraps a uniform reference.
///
template<class T>
VaryingRef<T>
Uniform(T& x)
{
    return VaryingRef<T>(&x, 0);
}


OIIO_NAMESPACE_END
