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

// Description : Implementation of the common function template specializations.


#ifndef CRYINCLUDE_CRYCOMMON_FUNCTORBASEFUNCTION_H
#define CRYINCLUDE_CRYCOMMON_FUNCTORBASEFUNCTION_H
#pragma once


#include "IFunctorBase.h"

//////////////////////////////////////////////////////////////////////////
// Return type void
// No arguments.
template<>
class TFunctor<void (*)()>
    : public IFunctorBase
{
public:
    typedef void (* TFunctionType)();

    TFunctor(TFunctionType pFunction)
        : m_pfnFunction(pFunction){}

    virtual void Call()
    {
        m_pfnFunction();
    }
private:
    TFunctionType                   m_pfnFunction;
};
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Return type void
// 1 argument.
template<typename tArgument1>
class TFunctor<void (*)(tArgument1)>
    : public IFunctorBase
{
public:
    typedef void (* TFunctionType)(tArgument1);

    TFunctor(TFunctionType pFunction, tArgument1 Argument1)
        : m_pfnFunction(pFunction)
        , m_tArgument1(Argument1){}

    virtual void Call()
    {
        m_pfnFunction(m_tArgument1);
    }
private:
    TFunctionType                   m_pfnFunction;
    tArgument1                      m_tArgument1;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Return type void
// 2 arguments.
template<typename tArgument1, typename tArgument2>
class TFunctor<void (*)(tArgument1, tArgument2)>
    : public IFunctorBase
{
public:
    typedef void (* TFunctionType)(tArgument1, tArgument2);

    TFunctor(TFunctionType pFunction, const tArgument1& Argument1, const tArgument2& Argument2)
        : m_pfnFunction(pFunction)
        , m_tArgument1(Argument1)
        , m_tArgument2(Argument2){}

    virtual void Call()
    {
        m_pfnFunction(m_tArgument1, m_tArgument2);
    }
private:
    TFunctionType                       m_pfnFunction;
    tArgument1                          m_tArgument1;
    tArgument2                          m_tArgument2;
};
//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_CRYCOMMON_FUNCTORBASEFUNCTION_H
