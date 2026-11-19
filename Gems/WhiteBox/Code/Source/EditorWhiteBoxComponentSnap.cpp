/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// EditorWhiteBoxComponent - publishes the mesh to the editor's viewport snapper
// (ViewportSnapSourceRequestBus).

#include "EditorWhiteBoxComponent.h"

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Debug/Profiler.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    AZ::Vector3 EditorWhiteBoxComponent::SnapWorldPosition(
        const AZ::Vector3& localPosition, const AZ::Vector3& nonUniformScale) const
    {
        // Uniform scale comes from the transform; Non-Uniform Scale is applied here, as in GetWorldBounds.
        return m_worldFromLocal.TransformPoint(localPosition * nonUniformScale);
    }

    void EditorWhiteBoxComponent::RebuildSnapVertexCache() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // clear() keeps the capacity, so per-frame rebuilds during a drag don't reallocate.
        m_snapVertexCache.clear();
        m_snapVertexCacheBounds = AZ::Aabb::CreateNull();
        m_snapVertexCacheValid = true;

        // Snap to what is displayed (EvaluatedMesh); null while the entity is still activating.
        WhiteBoxMesh* mesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
        if (mesh == nullptr)
        {
            return;
        }

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        const Api::VertexHandles vertexHandles = Api::MeshVertexHandles(*mesh);
        m_snapVertexCache.reserve(vertexHandles.size());

        const AZ::EntityId entityId = GetEntityId();
        for (const Api::VertexHandle vertexHandle : vertexHandles)
        {
            const AZ::Vector3 worldPosition =
                SnapWorldPosition(Api::VertexPosition(*mesh, vertexHandle), nonUniformScale);

            // The handle index lets a tool exclude the vertex it is dragging.
            m_snapVertexCache.emplace_back(worldPosition, entityId, aznumeric_cast<AZ::s64>(vertexHandle.Index()));
            m_snapVertexCacheBounds.AddPoint(worldPosition);
        }
    }

    AZ::Aabb EditorWhiteBoxComponent::GetSnapVertexBounds() const
    {
        // From the snap cache: GetWorldBounds would walk every vertex again on each change. A null
        // Aabb while the mesh is not ready tells the snapper to skip it.
        if (!m_snapVertexCacheValid)
        {
            RebuildSnapVertexCache();
        }

        return m_snapVertexCacheBounds;
    }

    void EditorWhiteBoxComponent::RebuildSnapEdgeCache() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_snapEdgeCache.clear();
        m_snapEdgeCacheValid = true;

        WhiteBoxMesh* mesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
        if (mesh == nullptr)
        {
            return;
        }

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        // Polygon border edges only, not the triangulation diagonals.
        const Api::EdgeHandles edgeHandles = Api::MeshPolygonEdgeHandles(*mesh);
        m_snapEdgeCache.reserve(edgeHandles.size());

        const AZ::EntityId entityId = GetEntityId();
        for (const Api::EdgeHandle edgeHandle : edgeHandles)
        {
            const AZStd::array<AZ::Vector3, 2> positions = Api::EdgeVertexPositions(*mesh, edgeHandle);

            m_snapEdgeCache.emplace_back(
                SnapWorldPosition(positions[0], nonUniformScale), SnapWorldPosition(positions[1], nonUniformScale),
                entityId, aznumeric_cast<AZ::s64>(edgeHandle.Index()));
        }
    }

    void EditorWhiteBoxComponent::RebuildSnapEdgeMidpointCache() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_snapEdgeMidpointCache.clear();
        m_snapEdgeMidpointCacheValid = true;

        WhiteBoxMesh* mesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
        if (mesh == nullptr)
        {
            return;
        }

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        const Api::EdgeHandles edgeHandles = Api::MeshPolygonEdgeHandles(*mesh);
        m_snapEdgeMidpointCache.reserve(edgeHandles.size());

        const AZ::EntityId entityId = GetEntityId();
        for (const Api::EdgeHandle edgeHandle : edgeHandles)
        {
            m_snapEdgeMidpointCache.emplace_back(
                SnapWorldPosition(Api::EdgeMidpoint(*mesh, edgeHandle), nonUniformScale), entityId,
                aznumeric_cast<AZ::s64>(edgeHandle.Index()));
        }
    }

    void EditorWhiteBoxComponent::RebuildSnapFaceCenterCache() const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_snapFaceCenterCache.clear();
        m_snapFaceCenterCacheValid = true;

        WhiteBoxMesh* mesh = const_cast<EditorWhiteBoxComponent*>(this)->EvaluatedMesh();
        if (mesh == nullptr)
        {
            return;
        }

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(
            nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        // Polygon centres, not triangle centres.
        const Api::PolygonHandles polygonHandles = Api::MeshPolygonHandles(*mesh);
        m_snapFaceCenterCache.reserve(polygonHandles.size());

        const AZ::EntityId entityId = GetEntityId();
        AZ::s64 polygonIndex = 0;
        for (const Api::PolygonHandle& polygonHandle : polygonHandles)
        {
            m_snapFaceCenterCache.emplace_back(
                SnapWorldPosition(Api::PolygonMidpoint(*mesh, polygonHandle), nonUniformScale), entityId,
                polygonIndex++);
        }
    }

    void EditorWhiteBoxComponent::CollectSnapEdges(
        const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapEdge>& out) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_snapEdgeCacheValid)
        {
            RebuildSnapEdgeCache();
        }

        for (const AzToolsFramework::ViewportSnapping::SnapEdge& edge : m_snapEdgeCache)
        {
            if (volume.IntersectsSegment(edge.m_start, edge.m_end))
            {
                out.push_back(edge);
            }
        }
    }

    void EditorWhiteBoxComponent::CollectSnapEdgeMidpoints(
        const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_snapEdgeMidpointCacheValid)
        {
            RebuildSnapEdgeMidpointCache();
        }

        for (const AzToolsFramework::ViewportSnapping::SnapVertex& midpoint : m_snapEdgeMidpointCache)
        {
            if (volume.Contains(midpoint.m_worldPosition))
            {
                out.push_back(midpoint);
            }
        }
    }

    void EditorWhiteBoxComponent::CollectSnapFaceCenters(
        const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_snapFaceCenterCacheValid)
        {
            RebuildSnapFaceCenterCache();
        }

        for (const AzToolsFramework::ViewportSnapping::SnapVertex& center : m_snapFaceCenterCache)
        {
            if (volume.Contains(center.m_worldPosition))
            {
                out.push_back(center);
            }
        }
    }

    void EditorWhiteBoxComponent::CollectSnapVertices(
        const AzToolsFramework::ViewportSnapping::SnapQueryVolume& volume, AZStd::vector<AzToolsFramework::ViewportSnapping::SnapVertex>& out) const
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!m_snapVertexCacheValid)
        {
            RebuildSnapVertexCache();
        }

        // Cached, already-transformed positions: no allocation or per-vertex API calls per query.
        for (const AzToolsFramework::ViewportSnapping::SnapVertex& vertex : m_snapVertexCache)
        {
            if (volume.Contains(vertex.m_worldPosition))
            {
                out.push_back(vertex);
            }
        }
    }
} // namespace WhiteBox
