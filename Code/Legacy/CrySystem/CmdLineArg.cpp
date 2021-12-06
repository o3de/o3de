/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "CmdLineArg.h"


CCmdLineArg::CCmdLineArg(const char* name, const char* value, ECmdLineArgType type)
{
    m_name = name;
    m_value = value;
    m_type = type;
}

CCmdLineArg::~CCmdLineArg()
{
}

const char* CCmdLineArg::GetName() const
{
    return m_name.c_str();
}
const char* CCmdLineArg::GetValue() const
{
    return m_value.c_str();
}
const ECmdLineArgType CCmdLineArg::GetType() const
{
    return m_type;
}
const float CCmdLineArg::GetFValue() const
{
    return (float)atof(m_value.c_str());
}
const int CCmdLineArg::GetIValue() const
{
    return atoi(m_value.c_str());
}
const bool CCmdLineArg::GetBoolValue(bool& cmdLineValue) const
{
    AZStd::string lowercaseValue(m_value);
    AZStd::to_lower(lowercaseValue.begin(), lowercaseValue.end());
    if (lowercaseValue == "true")
    {
        cmdLineValue = true;
        return true;
    }

    if (lowercaseValue == "false")
    {
        cmdLineValue = false;
        return true;
    }
    
    return false;
}

