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

#ifndef CRYINCLUDE_CRYCOMMON_NAME_TYPEINFO_H
#define CRYINCLUDE_CRYCOMMON_NAME_TYPEINFO_H
#pragma once

#include "CryName.h"

// CCryName TypeInfo

TYPE_INFO_BASIC(CCryName)

string ToString(CCryName const& val)
{
    return string(val.c_str());
}
bool FromString(CCryName& val, const char* s)
{
    val = s;
    return true;
}

#endif // CRYINCLUDE_CRYCOMMON_NAME_TYPEINFO_H
