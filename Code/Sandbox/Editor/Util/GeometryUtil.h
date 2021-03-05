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

// Description : Geometry utilities


#ifndef CRYINCLUDE_EDITOR_UTIL_GEOMETRYUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GEOMETRYUTIL_H
#pragma once
void ConvexHull2DGraham(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn);
//! Generates 2D convex hull from ptsIn using Andrew's algorithm.
SANDBOX_API void ConvexHull2DAndrew(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn);

//! Generates 2D convex hull from ptsIn
inline void ConvexHull2D(std::vector<Vec3>& ptsOut, const std::vector<Vec3>& ptsIn)
{
    // [Mikko] Note: The convex hull calculation is bound by the sorting.
    // The sort in Andrew's seems to be about 3-4x faster than Graham's--using Andrew's for now.
    ConvexHull2DAndrew(ptsOut, ptsIn);
}
#endif // CRYINCLUDE_EDITOR_UTIL_GEOMETRYUTIL_H
