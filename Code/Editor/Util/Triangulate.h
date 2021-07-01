/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
