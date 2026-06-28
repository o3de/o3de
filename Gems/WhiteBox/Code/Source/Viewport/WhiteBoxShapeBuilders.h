/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <EditorWhiteBoxDefaultShapeTypes.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Free geometry helpers used by DrawShapeMode to build white box shape solids
    //! (boxes, prisms, cones/pyramids, cylinders, spheres and staircases) from a drawn
    //! footprint. Declared here so DrawShapeMode.cpp keeps only their definitions and so
    //! they can be reused/tested. All faces are wound outward.
    namespace Detail
    {
        //! Build a right-handed basis with @p n (the surface normal) as the up axis.
        void BasisFromNormal(const AZ::Vector3& n, AZ::Vector3& right, AZ::Vector3& fwd, AZ::Vector3& up);

        //! Re-assign a staircase's in-plane axes so the same drawn footprint is re-oriented
        //! in 90-degree steps (which axis is the run and which way it climbs).
        void ApplyStairRotation(AZ::Vector3& uAxis, AZ::Vector3& vAxis, int rotation);

        //! Human-readable name for a draw shape (diagnostics).
        const char* DrawShapeName(DrawShapeType shape);

        //! Add one convex, planar face from an ordered loop of vertex handles, fan-triangulated
        //! and wound so its normal points away from @p solidCentroid (outward).
        void AddOutwardFace(
            WhiteBoxMesh& whiteBox, const AZStd::vector<Api::VertexHandle>& handles,
            const AZStd::vector<AZ::Vector3>& localPositions, const AZStd::vector<AZ::u32>& loop,
            const AZ::Vector3& solidCentroid);

        //! Add a round cap as one polygon, triangulated as an umbrella from a centre vertex
        //! (uniform triangles, no slivers), wound outward relative to @p solidCentroid.
        void AddOutwardCap(
            WhiteBoxMesh& whiteBox, const AZStd::vector<Api::VertexHandle>& handles,
            const AZStd::vector<AZ::Vector3>& localPositions, const AZStd::vector<AZ::u32>& rim,
            AZ::u32 centerIdx, const AZ::Vector3& solidCentroid);

        //! Generate the N-gon footprint ring (world space): round shapes inscribe the ellipse,
        //! angular shapes fill the rectangle. ru/rv are the half-extent vectors along the axes.
        AZStd::vector<AZ::Vector3> ComputeFootprintRing(
            const AZ::Vector3& center, const AZ::Vector3& ru, const AZ::Vector3& rv, bool round, int sidesIn);

        //! Build a watertight stepped staircase solid rising along +v over [baseUp, topUp].
        //! Only the outer shell is emitted, on a shared (N+1)x(N+1) vertex grid (manifold, no
        //! T-junctions).
        void BuildStaircaseSolid(
            WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld, const AZ::Vector3& center,
            const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up, float baseUp, float topUp,
            int stepsIn);

        //! Build a UV ellipsoid ("sphere") inscribed in the drawn footprint: in-plane radii from
        //! uAxis/vAxis, vertical radius from the [baseUp, topUp] span. @p segmentsIn drives the
        //! longitude resolution; latitude rings derive from it.
        void BuildSphereSolid(
            WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld, const AZ::Vector3& center,
            const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up, float baseUp, float topUp,
            int segmentsIn);

        //! Build a closed shape solid from a footprint (centre + in-plane axes) extruded along
        //! @p up between the base plane and the top plane/apex. Dispatches to the dedicated
        //! sphere/staircase builders or builds a prism/pyramid. All faces wound outward.
        void BuildShapeSolid(
            WhiteBoxMesh& mesh, const AZ::Transform& localFromWorld, const AZ::Vector3& center,
            const AZ::Vector3& uAxis, const AZ::Vector3& vAxis, const AZ::Vector3& up, float baseUp, float topUp,
            DrawShapeType shapeType, int sidesIn, int steps = 8);
    } // namespace Detail
} // namespace WhiteBox
