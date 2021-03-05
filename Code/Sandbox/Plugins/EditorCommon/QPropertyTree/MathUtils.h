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

#ifndef CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_MATHUTILS_H
#define CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_MATHUTILS_H
#pragma once

inline int xround(float v)
{
    return int(v + 0.5f);
}

inline int min(int a, int b)
{
    return a < b ? a : b;
}

inline int max(int a, int b)
{
    return a > b ? a : b;
}

inline float min(float a, float b)
{
    return a < b ? a : b;
}

inline float max(float a, float b)
{
    return a > b ? a : b;
}

inline float clamp(float value, float min, float max)
{
    return ::min(::max(min, value), max);
}

inline int clamp(int value, int min, int max)
{
    return ::min(::max(min, value), max);
}


#endif // CRYINCLUDE_EDITORCOMMON_QPROPERTYTREE_MATHUTILS_H
