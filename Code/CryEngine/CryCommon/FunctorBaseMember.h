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
