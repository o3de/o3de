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
