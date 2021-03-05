// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////
/// \file
///
/// Wrappers and utilities for reference counting.
/////////////////////////////////////////////////////////////////////////


#pragma once

#include <memory>

#include <OpenImageIO/atomic.h>
#include <OpenImageIO/dassert.h>


OIIO_NAMESPACE_BEGIN

using std::shared_ptr;  // DEPRECATED(1.8)



/// A simple intrusive pointer, modeled after std::shared_ptr.
template<class T> class intrusive_ptr {
public:
    typedef T element_type;

    /// Default ctr
    intrusive_ptr() noexcept
        : m_ptr(NULL)
    {
    }

    /// Construct from a raw pointer (presumed to be just now allocated,
    /// and now owned by us).
    intrusive_ptr(T* ptr)
        : m_ptr(ptr)
    {
        if (m_ptr)
            intrusive_ptr_add_ref(m_ptr);
    }

    /// Construct from another intrusive_ptr.
    intrusive_ptr(const intrusive_ptr& r)
        : m_ptr(r.get())
    {
        if (m_ptr)
            intrusive_ptr_add_ref(m_ptr);
    }

    /// Move construct from another intrusive_ptr.
    intrusive_ptr(intrusive_ptr&& r) noexcept
        : m_ptr(r.get())
    {
        r.m_ptr = NULL;
    }

    /// Destructor
    ~intrusive_ptr()
    {
        if (m_ptr)
            intrusive_ptr_release(m_ptr);
    }

    /// Assign from intrusive_ptr
    intrusive_ptr& operator=(const intrusive_ptr& r)
    {
        intrusive_ptr(r).swap(*this);
        return *this;
    }

    /// Move assignment from intrusive_ptr
    intrusive_ptr& operator=(intrusive_ptr&& r) noexcept
    {
        intrusive_ptr(static_cast<intrusive_ptr&&>(r)).swap(*this);
        return *this;
    }

    /// Reset to null reference
    void reset() noexcept
    {
        if (m_ptr) {
            intrusive_ptr_release(m_ptr);
            m_ptr = NULL;
        }
    }

    /// Reset to point to a pointer
    void reset(T* r)
    {
        if (r != m_ptr) {
            if (r)
                intrusive_ptr_add_ref(r);
            if (m_ptr)
                intrusive_ptr_release(m_ptr);
            m_ptr = r;
        }
    }

    /// Set this smart pointer to null, decrement the object's reference
    /// count, return the original raw pointer, but do NOT delete the object
    /// even if the ref count goes to zero. The only safe use case is to
    /// convert the sole managed pointer to an object into a raw pointer.
    /// DANGER -- use with caution! This is only safe to do if no other
    /// intrusive_ptr refers to the object (such a pointer may subsequently
    /// reset, decrementing the count to 0, and incorrectly free the
    /// object), and it can cause a memory leak if the caller isn't careful
    /// to either reassign the returned pointer to another managed pointer
    /// or delete it manually.
    T* release()
    {
        T* p = m_ptr;
        if (p) {
            if (!p->_decref())
                OIIO_DASSERT(0 && "release() when you aren't the sole owner");
            m_ptr = nullptr;
        }
        return p;
    }

    /// Swap intrusive pointers
    void swap(intrusive_ptr& r) noexcept
    {
        T* tmp  = m_ptr;
        m_ptr   = r.m_ptr;
        r.m_ptr = tmp;
    }

    /// Dereference
    T& operator*() const
    {
        OIIO_DASSERT(m_ptr);
        return *m_ptr;
    }

    /// Dereference
    T* operator->() const
    {
        OIIO_DASSERT(m_ptr);
        return m_ptr;
    }

    /// Get raw pointer
    T* get() const noexcept { return m_ptr; }

    /// Cast to bool to detect whether it points to anything
    operator bool() const noexcept { return m_ptr != NULL; }

private:
    T* m_ptr;  // the raw pointer
};



/// Mix-in class that adds a reference count, implemented as an atomic
/// counter.
class RefCnt {
protected:
    // Declare RefCnt constructors and destructors protected because they
    // should only be called implicitly from within child class constructors or
    // destructors.  In particular, this prevents users from deleting a RefCnt*
    // which is important because the destructor is non-virtual.

    RefCnt() { m_refcnt = 0; }

    /// Define copy constructor to NOT COPY reference counts! Copying a
    /// struct doesn't change how many other things point to it.
    RefCnt(RefCnt&) { m_refcnt = 0; }

    ~RefCnt() {}

public:
    /// Add a reference
    ///
    void _incref() const { ++m_refcnt; }

    /// Delete a reference, return true if that was the last reference.
    ///
    bool _decref() const { return (--m_refcnt) == 0; }

    /// Define operator= to NOT COPY reference counts!  Assigning a struct
    /// doesn't change how many other things point to it.
    const RefCnt& operator=(const RefCnt&) const { return *this; }

private:
    mutable atomic_int m_refcnt;
};



/// Implementation of intrusive_ptr_add_ref, which is needed for
/// any class that you use with intrusive_ptr.
template<class T>
inline void
intrusive_ptr_add_ref(T* x)
{
    x->_incref();
}

/// Implementation of intrusive_ptr_release, which is needed for
/// any class that you use with intrusive_ptr.
template<class T>
inline void
intrusive_ptr_release(T* x)
{
    if (x->_decref())
        delete x;
}

// Note that intrusive_ptr_add_ref and intrusive_ptr_release MUST be a
// templated on the full type, so that they pass the right address to
// 'delete' and destroy the right type.  If you try to just
// 'inline void intrusive_ptr_release (RefCnt *x)', that might seem
// clever, but it will end up getting the address of (and destroying)
// just the inherited RefCnt sub-object, not the full subclass you
// meant to delete and destroy.



// Preprocessor flags for some capabilities added incrementally.
#define OIIO_REFCNT_HAS_RELEASE 1 /* intrusive_ptr::release() */


OIIO_NAMESPACE_END
