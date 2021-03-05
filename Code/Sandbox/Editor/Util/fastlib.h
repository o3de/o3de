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

#ifndef CRYINCLUDE_EDITOR_UTIL_FASTLIB_H
#define CRYINCLUDE_EDITOR_UTIL_FASTLIB_H
#pragma once

__forceinline int RoundFloatToInt(float fValue)
{
    return (int)(fValue + 0.5f);
}

__forceinline int __stdcall FloatToIntRet(float fValue)
{
    return (int)(fValue + 0.5f);
}

__forceinline int ftoi(float fValue)
{
    return (int)(fValue);
}

__forceinline unsigned int __stdcall ifloor(float fValue)
{
    return ftoi(floor(fValue));
}

#endif // CRYINCLUDE_EDITOR_UTIL_FASTLIB_H
