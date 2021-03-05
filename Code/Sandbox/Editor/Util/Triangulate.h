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

#ifndef CRYINCLUDE_EDITOR_UTIL_TRIANGULATE_H
#define CRYINCLUDE_EDITOR_UTIL_TRIANGULATE_H
#pragma once

#include <vector>
#include "Cry_Vector3.h"

// you pass in a vector of vec3 (the contour of a shape) and it outputs a vector of vec3 (being the triangles)

namespace Triangulator
{
    typedef std::vector< Vec3 > VectorOfVectors;
    bool Triangulate(const VectorOfVectors& contour, VectorOfVectors& result);
};


#endif