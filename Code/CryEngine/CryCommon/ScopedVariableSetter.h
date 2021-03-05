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

#ifndef CRYINCLUDE_CRYCOMMON_SCOPEDVARIABLESETTER_H
#define CRYINCLUDE_CRYCOMMON_SCOPEDVARIABLESETTER_H
#pragma once


// The idea of this class is to set the value of a variable in its constructor,
// and then in the destructor to reset it to its initial value.
template <typename T>
class CScopedVariableSetter
{
public:
    typedef T Value;

    CScopedVariableSetter(Value& variable, const Value& temporaryValue)
        :   m_oldValue(variable)
        , m_variable(variable)
    {
        m_variable = temporaryValue;
    }

    ~CScopedVariableSetter()
    {
        m_variable = m_oldValue;
    }

private:
    Value m_oldValue;
    Value& m_variable;
};

#endif // CRYINCLUDE_CRYCOMMON_SCOPEDVARIABLESETTER_H
