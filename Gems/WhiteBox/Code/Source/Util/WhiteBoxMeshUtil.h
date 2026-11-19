/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxDefaultShapeTypes.h"
#include "Rendering/WhiteBoxMaterial.h"
#include "Viewport/WhiteBoxShapeBuilders.h"
#include "Rendering/WhiteBoxRenderData.h"

#include <AzCore/Math/Vector3.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Mesh-level helpers shared by the EditorWhiteBoxComponent translation units.
    //! Extracted from EditorWhiteBoxComponent.cpp (previously file-static).

    //! Append every polygon of @p src to @p dest as a separate island (not a CSG merge).
    void AppendMesh(WhiteBoxMesh& dest, const WhiteBoxMesh& src);

    //! Combine a freeform mesh with the cube grid into one output mesh (a plain append - no
    //! CSG). Any argument may be null/empty. Returns null only if there is nothing to combine.
    Api::WhiteBoxMeshPtr CombineFreeformAndGrids(WhiteBoxMesh* freeform, WhiteBoxMesh* grid);

    //! Remove every face and vertex of @p mesh in place (works for both the component-local
    //! working mesh and an asset-backed mesh).
    void ClearMeshInPlace(WhiteBoxMesh& mesh);

    //! Restore @p target from @p snapshot (an Api::CloneMesh taken at mouse-down), topology included, to
    //! revert a cancelled drag. False, leaving @p target untouched, if the copy failed.
    bool RestoreMeshFromSnapshot(WhiteBoxMesh& target, const WhiteBoxMesh& snapshot);

    //! A copy of @p src with every face's winding reversed, for Invert Normals: flipping the mesh itself
    //! keeps render, physics (single-sided) and selection in agreement.
    Api::WhiteBoxMeshPtr FlippedMeshWinding(const WhiteBoxMesh& src);

    //! @p src plus its reflection across the plane normal to @p axis (0 X, 1 Y, 2 Z) through the origin, or the
    //! mesh's low side when it straddles the origin. Polygons on the plane are dropped; the halves are not welded.
    Api::WhiteBoxMeshPtr MirrorMesh(const WhiteBoxMesh& src, int axis);

    //! @p count copies of @p src, each @p offset further along than the last (the first stays in place).
    Api::WhiteBoxMeshPtr ArrayMesh(const WhiteBoxMesh& src, int count, const AZ::Vector3& offset);

    //! Build a render face from a face handle, optionally reversed to render inside-out.
    //! @param withBlend read the corners' vertex-blend weights.
    WhiteBoxFace BuildWhiteBoxFace(
        const WhiteBoxMesh& whiteBox, const Api::FaceHandle& faceHandle, bool flipWinding, bool withBlend = true);

    //! Build intermediate render data (faces + material) passed to WhiteBoxRenderMeshInterface
    //! to generate the concrete render mesh.
    WhiteBoxRenderData CreateWhiteBoxRenderData(
        const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material, bool flipWinding = false);

    //! Generate a parametric shape centred on the origin in XY, from z=0 up to @p height (full extents).
    //! Room sizes are interior; Door and CircularStairs reuse width/depth/height as their own dimensions.
    Api::WhiteBoxMeshPtr BuildParametricShapeMesh(
        DrawShapeType shape, float width, float depth, float height, int sides, int steps,
        float wallThickness = 0.15f, float cavityGap = 0.1f, bool floor = true, bool ceiling = false,
        bool doorFrame = true, float archHeight = 0.0f, float innerRadius = 0.5f, float sweepAngle = 270.0f,
        bool stepsByHeight = false, float stepHeight = 0.25f, float holeRatio = 0.5f,
        int tubeSides = DefaultTubeSides);

    //! True when the default shape is set to a custom mesh asset.
    bool DisplayingAsset(DefaultShapeType defaultShapeType);

    //! Ask the property grid to refresh its attributes and values.
    void RefreshProperties();
} // namespace WhiteBox
