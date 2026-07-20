/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxMeshUtil.h"

#include "Viewport/WhiteBoxShapeBuilders.h"

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace WhiteBox
{
    void AppendMesh(WhiteBoxMesh& dest, const WhiteBoxMesh& src)
    {
        AZStd::unordered_map<int, Api::VertexHandle> vmap;
        const auto destVertex = [&](const Api::VertexHandle& srcVertex) -> Api::VertexHandle
        {
            const auto it = vmap.find(srcVertex.Index());
            if (it != vmap.end())
            {
                return it->second;
            }
            const Api::VertexHandle dh = Api::AddVertex(dest, Api::VertexPosition(src, srcVertex));
            vmap.emplace(srcVertex.Index(), dh);
            return dh;
        };

        for (const Api::PolygonHandle& polygon : Api::MeshPolygonHandles(src))
        {
            Api::FaceVertHandlesList faceVertHandles;
            faceVertHandles.reserve(polygon.m_faceHandles.size());
            for (const Api::FaceHandle& faceHandle : polygon.m_faceHandles)
            {
                const auto halfedges = Api::FaceHalfedgeHandles(src, faceHandle);
                if (halfedges.size() >= 3)
                {
                    faceVertHandles.push_back(Api::FaceVertHandles{
                        { destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[0])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[1])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[2])) }});
                }
            }
            if (!faceVertHandles.empty())
            {
                Api::AddPolygon(dest, faceVertHandles);
            }
        }
    }

    Api::WhiteBoxMeshPtr CombineFreeformAndGrids(WhiteBoxMesh* freeform, WhiteBoxMesh* grid)
    {
        const bool hasFreeform = freeform != nullptr && !Api::MeshFaceHandles(*freeform).empty();
        const bool hasSeparate = grid != nullptr && !Api::MeshFaceHandles(*grid).empty();
        if (!hasFreeform && !hasSeparate)
        {
            return nullptr;
        }
        Api::WhiteBoxMeshPtr combined = hasFreeform ? Api::CloneMesh(*freeform) : Api::CreateWhiteBoxMesh();
        if (!combined)
        {
            return nullptr;
        }
        if (hasSeparate)
        {
            AppendMesh(*combined, *grid);
        }
        Api::CalculateNormals(*combined);
        Api::CalculatePlanarUVs(*combined);
        return combined;
    }

    void ClearMeshInPlace(WhiteBoxMesh& mesh)
    {
        Api::RemoveFaces(mesh, Api::MeshFaceHandles(mesh));
        Api::RemoveIsolatedVertices(mesh);
    }

    Api::WhiteBoxMeshPtr FlippedMeshWinding(const WhiteBoxMesh& src)
    {
        Api::WhiteBoxMeshPtr dest = Api::CreateWhiteBoxMesh();
        AZStd::unordered_map<int, Api::VertexHandle> vmap;
        const auto destVertex = [&](const Api::VertexHandle& srcVertex) -> Api::VertexHandle
        {
            const auto it = vmap.find(srcVertex.Index());
            if (it != vmap.end())
            {
                return it->second;
            }
            const Api::VertexHandle dh = Api::AddVertex(*dest, Api::VertexPosition(src, srcVertex));
            vmap.emplace(srcVertex.Index(), dh);
            return dh;
        };

        for (const Api::PolygonHandle& polygon : Api::MeshPolygonHandles(src))
        {
            Api::FaceVertHandlesList faceVertHandles;
            faceVertHandles.reserve(polygon.m_faceHandles.size());
            for (const Api::FaceHandle& faceHandle : polygon.m_faceHandles)
            {
                const auto halfedges = Api::FaceHalfedgeHandles(src, faceHandle);
                if (halfedges.size() >= 3)
                {
                    // Reversed vertex order flips the triangle's winding (and thus its normal).
                    faceVertHandles.push_back(Api::FaceVertHandles{
                        { destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[2])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[1])),
                          destVertex(Api::HalfedgeVertexHandleAtTip(src, halfedges[0])) }});
                }
            }
            if (!faceVertHandles.empty())
            {
                Api::AddPolygon(*dest, faceVertHandles);
            }
        }

        if (!Api::MeshFaceHandles(*dest).empty())
        {
            Api::CalculateNormals(*dest);
            Api::CalculatePlanarUVs(*dest);
        }
        return dest;
    }

    WhiteBoxFace BuildWhiteBoxFace(
        const WhiteBoxMesh& whiteBox, const Api::FaceHandle& faceHandle, const bool flipWinding)
    {
        const auto copyVertex = [&whiteBox](const Api::HalfedgeHandle& in, WhiteBoxVertex& out)
        {
            const auto vh = Api::HalfedgeVertexHandleAtTip(whiteBox, in);
            out.m_position = Api::VertexPosition(whiteBox, vh);
            out.m_uv = Api::HalfedgeUV(whiteBox, in);
        };

        WhiteBoxFace face;
        face.m_normal = Api::FaceNormal(whiteBox, faceHandle);
        const auto faceHalfedgeHandles = Api::FaceHalfedgeHandles(whiteBox, faceHandle);

        if (flipWinding)
        {
            // Reverse winding (swap v1/v3) and flip the face normal so the layer renders inside-out.
            copyVertex(faceHalfedgeHandles[0], face.m_v3);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v1);
            face.m_normal = -face.m_normal;
        }
        else
        {
            copyVertex(faceHalfedgeHandles[0], face.m_v1);
            copyVertex(faceHalfedgeHandles[1], face.m_v2);
            copyVertex(faceHalfedgeHandles[2], face.m_v3);
        }

        return face;
    }

    WhiteBoxRenderData CreateWhiteBoxRenderData(
        const WhiteBoxMesh& whiteBox, const WhiteBoxMaterial& material, const bool flipWinding)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxRenderData renderData;
        WhiteBoxFaces& faceData = renderData.m_faces;

        const auto faceCount = Api::MeshFaceCount(whiteBox);
        faceData.reserve(faceCount);

        const auto faceHandles = Api::MeshFaceHandles(whiteBox);
        for (const auto& faceHandle : faceHandles)
        {
            faceData.push_back(BuildWhiteBoxFace(whiteBox, faceHandle, flipWinding));
        }

        renderData.m_material = material;

        return renderData;
    }

    Api::WhiteBoxMeshPtr BuildParametricShapeMesh(
        const DrawShapeType shape, const float width, const float depth, const float height, const int sides,
        const int steps, const float wallThickness, const float cavityGap, const bool floor, const bool ceiling)
    {
        const float w = AZStd::max(width, 0.01f);
        const float d = AZStd::max(depth, 0.01f);
        const float h = AZStd::max(height, 0.01f);
        const int clampedSides = AZStd::clamp(sides, 3, 128);
        const int clampedSteps = AZStd::clamp(steps, 1, 128);

        Api::WhiteBoxMeshPtr mesh = Api::CreateWhiteBoxMesh();

        // The Room is a shell, not a footprint-extruded solid, so it has its own builder rather than
        // going through BuildShapeSolid. width/depth/height are its INTERIOR dimensions here.
        if (shape == DrawShapeType::Room)
        {
            Detail::BuildRoomSolid(*mesh, w, d, h, wallThickness, cavityGap, floor, ceiling);
        }
        else
        {
            // Note: the builders take FULL-extent axis vectors (they halve internally).
            Detail::BuildShapeSolid(
                *mesh, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateZero(),
                AZ::Vector3(w, 0.0f, 0.0f), AZ::Vector3(0.0f, d, 0.0f),
                AZ::Vector3::CreateAxisZ(), 0.0f, h, shape, clampedSides, clampedSteps);
        }
        if (!Api::MeshFaceHandles(*mesh).empty())
        {
            Api::CalculateNormals(*mesh);
            Api::CalculatePlanarUVs(*mesh);
        }
        return mesh;
    }

    bool DisplayingAsset(const DefaultShapeType defaultShapeType)
    {
        // checks if the default shape is set to a custom asset
        return defaultShapeType == DefaultShapeType::Asset;
    }

    void RefreshProperties()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
    }
} // namespace WhiteBox
