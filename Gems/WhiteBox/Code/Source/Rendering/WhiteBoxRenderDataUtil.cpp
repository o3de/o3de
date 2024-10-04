/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderData.h"
#include "WhiteBoxRenderDataUtil.h"
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>

namespace WhiteBox
{
    AzFramework::VisibleGeometry BuildVisibleGeometryFromWhiteBoxRenderData(
        const AZ::EntityId& entityId, const WhiteBoxRenderData& renderData)
    {
        // Calculate the world transform for the entity which will be used to transform the white box geometry
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, entityId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);

        // Initialize the visible geometry structure by reserving memory for mesh data and setting its world transform
        const WhiteBoxFaces faceData = BuildCulledWhiteBoxFaces(renderData.m_faces);
        const size_t faceCount = faceData.size();
        const size_t vertCount = faceCount * 3;

        AzFramework::VisibleGeometry geometry;
        geometry.m_vertices.reserve(vertCount * 3);
        geometry.m_indices.reserve(vertCount);
        geometry.m_transform = AZ::Matrix4x4::CreateFromTransform(transform);
        geometry.m_transform *= AZ::Matrix4x4::CreateScale(nonUniformScale);
        geometry.m_transparent = false;

        // Copy white box render data vertices and indices into visible geometry structure. This is not as efficient as it could be. The
        // function that builds the white box data removes degenerate triangles but the format does not allow sharing vertices and edges.
        uint32_t vertIndex = 0;
        for (const auto& face : faceData)
        {
            geometry.m_vertices.push_back(face.m_v1.m_position.GetX());
            geometry.m_vertices.push_back(face.m_v1.m_position.GetY());
            geometry.m_vertices.push_back(face.m_v1.m_position.GetZ());
            geometry.m_indices.push_back(vertIndex++);

            geometry.m_vertices.push_back(face.m_v2.m_position.GetX());
            geometry.m_vertices.push_back(face.m_v2.m_position.GetY());
            geometry.m_vertices.push_back(face.m_v2.m_position.GetZ());
            geometry.m_indices.push_back(vertIndex++);

            geometry.m_vertices.push_back(face.m_v3.m_position.GetX());
            geometry.m_vertices.push_back(face.m_v3.m_position.GetY());
            geometry.m_vertices.push_back(face.m_v3.m_position.GetZ());
            geometry.m_indices.push_back(vertIndex++);
        }

        return geometry;
    }
} // namespace WhiteBox
