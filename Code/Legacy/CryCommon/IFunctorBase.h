/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Base header for multi DLL functors.


#ifndef CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
#define CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
#pragma once

#include <AzCore/std/parallel/atomic.h>

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
        m_nReferences.fetch_add(1, AZStd::memory_order_acq_rel);
    }

    void Release()
    {
        if (m_nReferences.fetch_sub(1, AZStd::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

protected:
    AZStd::atomic_int m_nReferences;
};

// Base Template for specialization.
// Not intended for direct usage.
template<typename tType>
class TFunctor
    : public IFunctorBase
{
};

#endif // CRYINCLUDE_CRYCOMMON_IFUNCTORBASE_H
