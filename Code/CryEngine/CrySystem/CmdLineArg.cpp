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


