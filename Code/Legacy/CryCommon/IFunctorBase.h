/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Base header for multi DLL functors.


#ifndef CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
#define CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
#pragma once


// Base class for functor storage.
// Not intended for direct usage.
class IFunctorBase
{
public:
    IFunctorBase()
        : m_nReferences(0){}
    virtual ~IFunctorBase(){};
    virtual void Call() = 0;

    void AddRef()
    {
        CryInterlockedIncrement(&m_nReferences);
    }

    void Release()
    {
        if (CryInterlockedDecrement(&m_nReferences) <= 0)
        {
            delete this;
        }
    }

protected:
    volatile int m_nReferences;
};

// Base Template for specialization.
// Not intended for direct usage.
template<typename tType>
class TFunctor
    : public IFunctorBase
{
};

#endif // CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
