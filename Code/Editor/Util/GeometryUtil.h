/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
