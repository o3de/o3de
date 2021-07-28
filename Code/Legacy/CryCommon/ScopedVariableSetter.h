/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
