/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
