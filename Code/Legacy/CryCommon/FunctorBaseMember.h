/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of the member function template specializations.

#ifndef CRYINCLUDE_CRYCOMMON_FUNCTORBASEMEMBER_H
#define CRYINCLUDE_CRYCOMMON_FUNCTORBASEMEMBER_H
#pragma once


#include "IFunctorBase.h"

//////////////////////////////////////////////////////////////////////////
// Return type void
// No Arguments
template<typename tCalleeType>
class TFunctor<void (tCalleeType::*)()>
    : public IFunctorBase
{
public:
    typedef void (tCalleeType::* TMemberFunctionType)();

    TFunctor(tCalleeType* pCallee, TMemberFunctionType pMemberFunction)
        : m_pCalee(pCallee)
        , m_pfnMemberFunction(pMemberFunction){}

    virtual void Call()
    {
        (m_pCalee->*m_pfnMemberFunction)();
    }
private:
    tCalleeType*                        m_pCalee;
    TMemberFunctionType         m_pfnMemberFunction;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Return type void
// 1 argument.
template<typename tCalleeType, typename tArgument1>
class TFunctor<void (tCalleeType::*)(tArgument1)>
    : public IFunctorBase
{
public:
    typedef void (tCalleeType::* TMemberFunctionType)(tArgument1);

    TFunctor(tCalleeType* pCallee, TMemberFunctionType pMemberFunction, const tArgument1& Argument1)
        : m_pCalee(pCallee)
        , m_pfnMemberFunction(pMemberFunction)
        , m_tArgument1(Argument1){}

    virtual void Call()
    {
        (m_pCalee->*m_pfnMemberFunction)(m_tArgument1);
    }
private:
    tCalleeType*                                    m_pCalee;
    TMemberFunctionType                     m_pfnMemberFunction;
    tArgument1                                      m_tArgument1;
};
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Return type void
// 2 arguments.
template<typename tCalleeType, typename tArgument1, typename tArgument2>
class TFunctor<void (tCalleeType::*)(tArgument1, tArgument2)>
    : public IFunctorBase
{
public:
    typedef void (tCalleeType::* TMemberFunctionType)(tArgument1, tArgument2);

    TFunctor(tCalleeType* pCallee, TMemberFunctionType pMemberFunction, const tArgument1& Argument1, const tArgument2& Argument2)
        : m_pCalee(pCallee)
        , m_pfnMemberFunction(pMemberFunction)
        , m_tArgument1(Argument1)
        , m_tArgument2(Argument2){}

    virtual void Call()
    {
        (m_pCalee->*m_pfnMemberFunction)(m_tArgument1, m_tArgument2);
    }
private:
    tCalleeType*                                    m_pCalee;
    TMemberFunctionType                     m_pfnMemberFunction;
    tArgument1                                      m_tArgument1;
    tArgument2                                      m_tArgument2;
};
//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_CRYCOMMON_FUNCTORBASEMEMBER_H
