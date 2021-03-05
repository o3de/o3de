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

// Description : Helper to enable inplace construction and destruction of objects

#ifndef CRYINCLUDE_CRYCOMMON_STACKCONTAINER_H
#define CRYINCLUDE_CRYCOMMON_STACKCONTAINER_H
#pragma once

#include <InplaceFactory.h>

// Class that contains a non-pod data type allocated on the stack via placement
// new. Constructor parameters up to an arity of 5 are forwarded over an
// inplace factory expression.
template<typename T>
class CStackContainer
{
    // The backing storage
    uint8 m_Storage[ sizeof (T) ];

    // Constructs the object via an inplace factory
    template<class Factory>
    void construct (const Factory& factory)
    {
        factory.template apply<T>(m_Storage);
    }

    // Destructs the object. The destructor of the contained object is called
    void destruct ()
    {
        reinterpret_cast<T*>(m_Storage)->~T();
    }

    // Prevent the object to be placed on the heap
    // (.... it simply wouldn't make much sense)
    void* operator new(size_t);
    void operator delete(void*);

public:

    // Constructs inside the object
    template<class Expr>
    CStackContainer (const Expr& expr)
    { construct(expr); }

    // Destructs the object contained
    ~CStackContainer() { destruct(); }

    // Accessor methods
    T* get() { return reinterpret_cast<T*>(m_Storage); }
    const T* get() const { return reinterpret_cast<const T*>(m_Storage); }
};

#endif // CRYINCLUDE_CRYCOMMON_STACKCONTAINER_H
