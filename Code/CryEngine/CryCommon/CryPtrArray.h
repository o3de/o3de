/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_CRYPTRARRAY_H
#define CRYINCLUDE_CRYCOMMON_CRYPTRARRAY_H
#pragma once

#include "CryArray.h"
#include "CrySizer.h"

//---------------------------------------------------------------------------
template<class T, class P = T*>
struct PtrArray
    : DynArray<P>
{
    typedef DynArray<P> super;

    // Overrides.
    typedef T value_type;

    ILINE ~PtrArray(){}

    inline T&   operator [](int i) const
    { return *super::operator[](i); }

    // Iterators.
    struct iterator
    {
        iterator(P* p)
            : _ptr(p)
        {}

        operator P* () const
        {
            return _ptr;
        }
        void operator++()
        { _ptr++; }
        void operator--()
        { _ptr--; }
        T& operator*() const
        { assert(_ptr); return **_ptr; }
        T* operator->() const
        { assert(_ptr); return *_ptr; }

    protected:
        P* _ptr;
    };

    struct const_iterator
    {
        const_iterator(const P* p)
            : _ptr(p)
        {}

        operator const P* () const
        {
            return _ptr;
        }
        void operator++()
        { _ptr++; }
        void operator--()
        { _ptr--; }
        T& operator*() const
        { assert(_ptr); return **_ptr; }
        T* operator->() const
        { assert(_ptr); return *_ptr; }

    protected:
        const P* _ptr;
    };

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this->begin(), this->get_alloc_size());
        for (int i = 0; i < this->size(); ++i)
        {
            pSizer->AddObject(this->super::operator [](i));
        }
    }
};

//---------------------------------------------------------------------------
template<class T>
struct SmartPtrArray
    : PtrArray< T, _smart_ptr<T> >
{
};

#endif // CRYINCLUDE_CRYCOMMON_CRYPTRARRAY_H
