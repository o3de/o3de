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

// Description : Implements the command line argument interface ICmdLineArg.


#ifndef CRYINCLUDE_CRYSYSTEM_CMDLINEARG_H
#define CRYINCLUDE_CRYSYSTEM_CMDLINEARG_H

#pragma once

#include <ICmdLine.h>


class CCmdLineArg
    : public ICmdLineArg
{
public:
    CCmdLineArg(const char* name, const char* value, ECmdLineArgType type);
    virtual ~CCmdLineArg();

    const char* GetName() const;
    const char* GetValue() const;
    const ECmdLineArgType GetType() const;
    const float GetFValue() const;
    const int GetIValue() const;

private:

    ECmdLineArgType m_type;
    string          m_name;
    string          m_value;
};

#endif // CRYINCLUDE_CRYSYSTEM_CMDLINEARG_H
