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

#ifndef CRYINCLUDE_CRYCOMMON_PRIMITIVES_INFO_H
#define CRYINCLUDE_CRYCOMMON_PRIMITIVES_INFO_H
#pragma once

#include "primitives.h"

STRUCT_INFO_TYPE_EMPTY(primitives::primitive)

STRUCT_INFO_BEGIN(primitives::box)
STRUCT_BASE_INFO(primitives::primitive)
STRUCT_VAR_INFO(Basis, TYPE_INFO(Matrix33))
STRUCT_VAR_INFO(bOriented, TYPE_INFO(int))
STRUCT_VAR_INFO(center, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(size, TYPE_INFO(Vec3))
STRUCT_INFO_END(primitives::box)

STRUCT_INFO_BEGIN(primitives::sphere)
STRUCT_BASE_INFO(primitives::primitive)
STRUCT_VAR_INFO(center, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(r, TYPE_INFO(float))
STRUCT_INFO_END(primitives::sphere)

STRUCT_INFO_BEGIN(primitives::cylinder)
STRUCT_BASE_INFO(primitives::primitive)
STRUCT_VAR_INFO(center, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(axis, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(r, TYPE_INFO(float))
STRUCT_VAR_INFO(hh, TYPE_INFO(float))
STRUCT_INFO_END(primitives::cylinder)

STRUCT_INFO_BEGIN(primitives::plane)
STRUCT_BASE_INFO(primitives::primitive)
STRUCT_VAR_INFO(n, TYPE_INFO(Vec3))
STRUCT_VAR_INFO(origin, TYPE_INFO(Vec3))
STRUCT_INFO_END(primitives::plane)

#endif // CRYINCLUDE_CRYCOMMON_PRIMITIVES_INFO_H
