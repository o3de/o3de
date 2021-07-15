/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
