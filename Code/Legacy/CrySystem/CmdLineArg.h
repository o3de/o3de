/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    const bool GetBoolValue(bool& cmdLineValue) const;

private:

    ECmdLineArgType m_type;
    AZStd::string   m_name;
    AZStd::string   m_value;
};

#endif // CRYINCLUDE_CRYSYSTEM_CMDLINEARG_H
