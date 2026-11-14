/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! A face corner and the UV it should take.
    using UvChange = AZStd::pair<Api::HalfedgeHandle, AZ::Vector2>;

    //! UV layout operations. Each computes new corner UVs for the given faces without touching the mesh, so a caller
    //! applies them the way any other UV edit is applied (and undone).
    namespace UvOps
    {
        //! Unfold the faces flat: every planar polygon keeps its true shape and neighbours hinge on shared edges, flattest
        //! hinges first; a polygon that would overlap starts a new island. The islands are then packed into the unit square.
        AZStd::vector<UvChange> Unwrap(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces, float margin = 0.01f);

        //! Arrange the faces' current UV islands in the unit square without overlap, keeping their relative sizes.
        //! Islands are faces joined through corners that share a vertex and a UV; tall ones are turned a quarter turn.
        AZStd::vector<UvChange> Pack(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces, float margin = 0.01f);

        //! Fit each island into the trim band @p v0..@p v1: long side along U, height filling the band less
        //! @p inset, starting at U 0 and running on past 1.
        AZStd::vector<UvChange> FitToBand(const WhiteBoxMesh& whiteBox, const Api::FaceHandles& faces, float v0, float v1, float inset = 0.0f);
    } // namespace UvOps
} // namespace WhiteBox
