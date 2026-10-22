/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "WhiteBoxCsgCore.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    namespace Api
    {
        //! Internal helpers backing the CSG integration layer (see WhiteBoxCsg.cpp).
        //! These convert between white box meshes and the engine-agnostic Csg::TriangleMesh,
        //! group/clean the coplanar regions a boolean leaves behind, and rebuild a white box
        //! mesh from a triangulated result. Declared here so they can be unit tested and so
        //! the implementation file stays focused on definitions.
        namespace Detail
        {
            //! A 2D point on a region's plane (double precision for robust geometry).
            struct V2
            {
                double x;
                double y;
            };

            //! Convert a white box mesh to an indexed triangle mesh, transforming all
            //! positions by the given transform. Shared vertices are welded.
            Csg::TriangleMesh ToTriangleMesh(const WhiteBoxMesh& whiteBox, const AZ::Transform& transform);

            //! Reconstruct the world-space position of a triangle-mesh vertex.
            AZ::Vector3 TriangleMeshPosition(const Csg::TriangleMesh& triangleMesh, uint32_t vertexIndex);

            //! Compute the (safe-normalized) normal of the given triangle.
            AZ::Vector3 TriangleNormal(const Csg::TriangleMesh& triangleMesh, size_t triangleIndex);

            //! Group triangles into connected coplanar regions; each region becomes one
            //! white box polygon (interior edges hidden).
            AZStd::vector<AZStd::vector<size_t>> GroupCoplanarTriangles(const Csg::TriangleMesh& triangleMesh);

            //! Twice the signed area of triangle (a, b, c); > 0 when CCW.
            double Cross2(const V2& a, const V2& b, const V2& c);

            //! Is point p inside (or on) triangle (a, b, c)?
            bool PointInTri(const V2& p, const V2& a, const V2& b, const V2& c);

            //! Ear-clip a simple CCW polygon (2D), emitting triangle index-triples into outTris.
            bool EarClip(const AZStd::vector<V2>& pts, AZStd::vector<AZStd::array<uint32_t, 3>>& outTris);

            //! Merge holes (CW) into the outer loop (CCW), producing one simple CCW polygon by
            //! inserting a 'bridge' edge per hole (classic ear-clipping hole handling).
            bool BridgeHoles(
                const AZStd::vector<V2>& outerPts, const AZStd::vector<uint32_t>& outerGid,
                AZStd::vector<AZStd::vector<V2>>& holePts, AZStd::vector<AZStd::vector<uint32_t>>& holeGid,
                AZStd::vector<V2>& mergedPts, AZStd::vector<uint32_t>& mergedGid);

            //! Find vertices that sit mid-way along a straight visible edge and are therefore
            //! redundant; removing them never opens a crack.
            AZStd::unordered_set<uint32_t> ComputeRemovableVertices(
                const Csg::TriangleMesh& mesh, const AZStd::vector<AZStd::vector<size_t>>& groups);

            //! Rebuild one coplanar group as clean triangles (global vertex indices) with
            //! redundant collinear vertices removed. Returns false for anything it can't
            //! safely simplify (the caller then keeps the original triangles).
            bool CleanCoplanarGroup(
                const Csg::TriangleMesh& mesh, const AZStd::vector<size_t>& group, const AZ::Vector3& groupNormal,
                const AZStd::unordered_set<uint32_t>& removable, AZStd::vector<AZStd::array<uint32_t, 3>>& outTris);

            //! Average (area-weighted) normal of a coplanar triangle group.
            AZ::Vector3 GroupNormal(const Csg::TriangleMesh& mesh, const AZStd::vector<size_t>& group);

            //! Replace the contents of the white box mesh with the given triangle mesh.
            void RebuildFromTriangleMesh(WhiteBoxMesh& whiteBox, const Csg::TriangleMesh& triangleMesh);
        } // namespace Detail
    } // namespace Api
} // namespace WhiteBox
