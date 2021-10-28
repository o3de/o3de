/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomActorDebugDraw.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Integration/Rendering/RenderActorInstance.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>

namespace AZ::Render
{
    AtomActorDebugDraw::AtomActorDebugDraw(AZ::EntityId entityId)
    {
        m_auxGeomFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<RPI::AuxGeomFeatureProcessorInterface>(entityId);
    }

    void AtomActorDebugDraw::DebugDraw(const EMotionFX::ActorRenderFlagBitset& renderFlags, EMotionFX::ActorInstance* instance)
    {
        if (!m_auxGeomFeatureProcessor || !instance)
        {
            return;
        }

        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();
        if (!auxGeom)
        {
            return;
        }

        // Render aabb
        if (renderFlags[EMotionFX::ActorRenderFlag::RENDER_AABB])
        {
            RenderAABB(instance);
        }

        // Render skeleton
        if (renderFlags[EMotionFX::ActorRenderFlag::RENDER_LINESKELETON])
        {
            RenderSkeleton(instance);
        }

        // Render internal EMFX debug lines.
        if (renderFlags[EMotionFX::ActorRenderFlag::RENDER_EMFX_DEBUG])
        {
            RenderEMFXDebugDraw(instance);
        }

        // Render vertex normal, face normal, tagent and wireframe.
        const bool renderVertexNormals = renderFlags[EMotionFX::ActorRenderFlag::RENDER_VERTEXNORMALS];
        const bool renderFaceNormals = renderFlags[EMotionFX::ActorRenderFlag::RENDER_FACENORMALS];
        const bool renderTangents = renderFlags[EMotionFX::ActorRenderFlag::RENDER_TANGENTS];
        const bool renderWireframe = renderFlags[EMotionFX::ActorRenderFlag::RENDER_WIREFRAME];

        if (renderVertexNormals || renderFaceNormals || renderTangents || renderWireframe)
        {
            // Iterate through all enabled nodes
            const EMotionFX::Pose* pose = instance->GetTransformData()->GetCurrentPose();
            const size_t geomLODLevel = instance->GetLODLevel();
            const size_t numEnabled = instance->GetNumEnabledNodes();
            for (size_t i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node* node = instance->GetActor()->GetSkeleton()->GetNode(instance->GetEnabledNode(i));
                EMotionFX::Mesh* mesh = instance->GetActor()->GetMesh(geomLODLevel, node->GetNodeIndex());
                const AZ::Transform globalTM = pose->GetWorldSpaceTransform(node->GetNodeIndex()).ToAZTransform();

                m_currentMesh = nullptr;

                if (!mesh)
                {
                    continue;
                }

                RenderNormals(mesh, globalTM, renderVertexNormals, renderFaceNormals);
                if (renderTangents)
                {
                    RenderTangents(mesh, globalTM);
                }
                if (renderWireframe)
                {
                    RenderWireframe(mesh, globalTM);
                }
            }
        }
    }

    void AtomActorDebugDraw::PrepareForMesh(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM)
    {
        // Check if we have already prepared for the given mesh
        if (m_currentMesh == mesh)
        {
            return;
        }

        // Set our new current mesh
        m_currentMesh = mesh;

        // Get the number of vertices and the data
        const uint32 numVertices = m_currentMesh->GetNumVertices();
        AZ::Vector3* positions = (AZ::Vector3*)m_currentMesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS);

        // Check if the vertices fits in our buffer
        if (m_worldSpacePositions.size() < numVertices)
        {
            m_worldSpacePositions.resize(numVertices);
        }

        // Pre-calculate the world space positions
        for (uint32 i = 0; i < numVertices; ++i)
        {
            m_worldSpacePositions[i] = worldTM.TransformPoint(positions[i]);
        }
    }

    void AtomActorDebugDraw::RenderAABB(EMotionFX::ActorInstance* instance)
    {
        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();
        const AZ::Aabb& aabb = instance->GetAabb();
        auxGeom->DrawAabb(aabb, AZ::Color(0.0f, 1.0f, 1.0f, 1.0f), RPI::AuxGeomDraw::DrawStyle::Line);
    }

    void AtomActorDebugDraw::RenderSkeleton(EMotionFX::ActorInstance* instance)
    {
        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();

        const EMotionFX::TransformData* transformData = instance->GetTransformData();
        const EMotionFX::Skeleton* skeleton = instance->GetActor()->GetSkeleton();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();

        const size_t lodLevel = instance->GetLODLevel();
        const size_t numJoints = skeleton->GetNumNodes();

        m_auxVertices.clear();
        m_auxVertices.reserve(numJoints * 2);

        for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex)
        {
            const EMotionFX::Node* joint = skeleton->GetNode(jointIndex);
            if (!joint->GetSkeletalLODStatus(lodLevel))
            {
                continue;
            }

            const size_t parentIndex = joint->GetParentIndex();
            if (parentIndex == InvalidIndex)
            {
                continue;
            }

            const AZ::Vector3 parentPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
            m_auxVertices.emplace_back(parentPos);

            const AZ::Vector3 bonePos = pose->GetWorldSpaceTransform(jointIndex).m_position;
            m_auxVertices.emplace_back(bonePos);
        }

        const AZ::Color skeletonColor(0.604f, 0.804f, 0.196f, 1.0f);
        RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
        lineArgs.m_verts = m_auxVertices.data();
        lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
        lineArgs.m_colors = &skeletonColor;
        lineArgs.m_colorCount = 1;
        lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
        auxGeom->DrawLines(lineArgs);
    }

    void AtomActorDebugDraw::RenderEMFXDebugDraw(EMotionFX::ActorInstance* instance)
    {
        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();

        EMotionFX::DebugDraw& debugDraw = EMotionFX::GetDebugDraw();
        debugDraw.Lock();
        EMotionFX::DebugDraw::ActorInstanceData* actorInstanceData = debugDraw.GetActorInstanceData(instance);
        actorInstanceData->Lock();
        const AZStd::vector<EMotionFX::DebugDraw::Line>& lines = actorInstanceData->GetLines();
        if (lines.empty())
        {
            actorInstanceData->Unlock();
            debugDraw.Unlock();
            return;
        }

        m_auxVertices.clear();
        m_auxVertices.reserve(lines.size() * 2);
        m_auxColors.clear();
        m_auxColors.reserve(m_auxVertices.size());

        for (const EMotionFX::DebugDraw::Line& line : actorInstanceData->GetLines())
        {
            m_auxVertices.emplace_back(line.m_start);
            m_auxColors.emplace_back(line.m_startColor);
            m_auxVertices.emplace_back(line.m_end);
            m_auxColors.emplace_back(line.m_endColor);
        }

        AZ_Assert(m_auxVertices.size() == m_auxColors.size(), "Number of vertices and number of colors need to match.");
        actorInstanceData->Unlock();
        debugDraw.Unlock();

        RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
        lineArgs.m_verts = m_auxVertices.data();
        lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
        lineArgs.m_colors = m_auxColors.data();
        lineArgs.m_colorCount = static_cast<uint32_t>(m_auxColors.size());
        lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
        auxGeom->DrawLines(lineArgs);
    }

    void AtomActorDebugDraw::RenderNormals(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, bool vertexNormals, bool faceNormals)
    {
        if (!mesh)
        {
            return;
        }

        if (!vertexNormals && !faceNormals)
        {
            return;
        }

        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();
        if (!auxGeom)
        {
            return;
        }

        // TODO: Move line color to a render setting.
        const float faceNormalsScale = 0.01f;
        const AZ::Color colorFaceNormals = AZ::Colors::Lime;
        const float vertexNormalsScale = 0.01f;
        const AZ::Color colorVertexNormals = AZ::Colors::Orange;

        PrepareForMesh(mesh, worldTM);

        AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);

        // Render face normals
        if (faceNormals)
        {
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                const uint32 numTriangles = subMesh->GetNumPolygons();
                const uint32 startVertex = subMesh->GetStartVertex();
                const uint32* indices = subMesh->GetIndices();

                m_auxVertices.clear();
                m_auxVertices.reserve(numTriangles * 2);

                for (uint32 triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
                {
                    const uint32 triangleStartIndex = triangleIndex * 3;
                    const uint32 indexA = indices[triangleStartIndex + 0] + startVertex;
                    const uint32 indexB = indices[triangleStartIndex + 1] + startVertex;
                    const uint32 indexC = indices[triangleStartIndex + 2] + startVertex;

                    const AZ::Vector3& posA = m_worldSpacePositions[indexA];
                    const AZ::Vector3& posB = m_worldSpacePositions[indexB];
                    const AZ::Vector3& posC = m_worldSpacePositions[indexC];

                    const AZ::Vector3 normalDir = (posB - posA).Cross(posC - posA).GetNormalized();

                    // Calculate the center pos
                    const AZ::Vector3 normalPos = (posA + posB + posC) * (1.0f / 3.0f);

                    m_auxVertices.emplace_back(normalPos);
                    m_auxVertices.emplace_back(normalPos + (normalDir * faceNormalsScale));
                }
            }

            RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
            lineArgs.m_verts = m_auxVertices.data();
            lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
            lineArgs.m_colors = &colorFaceNormals;
            lineArgs.m_colorCount = 1;
            lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
            auxGeom->DrawLines(lineArgs);
        }

        // render vertex normals
        if (vertexNormals)
        {
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                const uint32 numVertices = subMesh->GetNumVertices();
                const uint32 startVertex = subMesh->GetStartVertex();

                m_auxVertices.clear();
                m_auxVertices.reserve(numVertices * 2);

                for (uint32 j = 0; j < numVertices; ++j)
                {
                    const uint32 vertexIndex = j + startVertex;
                    const AZ::Vector3& position = m_worldSpacePositions[vertexIndex];
                    const AZ::Vector3 normal = worldTM.TransformVector(normals[vertexIndex]).GetNormalizedSafe() * vertexNormalsScale;

                    m_auxVertices.emplace_back(position);
                    m_auxVertices.emplace_back(position + normal);
                }
            }

            RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
            lineArgs.m_verts = m_auxVertices.data();
            lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
            lineArgs.m_colors = &colorVertexNormals;
            lineArgs.m_colorCount = 1;
            lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
            auxGeom->DrawLines(lineArgs);
        }
    }

    void AtomActorDebugDraw::RenderTangents(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM)
    {
        if (!mesh)
        {
            return;
        }

        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();
        if (!auxGeom)
        {
            return;
        }

        // TODO: Move line color to a render setting.
        const AZ::Color colorTangents = AZ::Colors::Red;
        const AZ::Color mirroredBitangentColor = AZ::Colors::Yellow;
        const AZ::Color colorBitangents = AZ::Colors::White;
        const float scale = 0.01f;

        // Get the tangents and check if this mesh actually has tangents
        AZ::Vector4* tangents = static_cast<AZ::Vector4*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
        if (!tangents)
        {
            return;
        }

        AZ::Vector3* bitangents = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));

        PrepareForMesh(mesh, worldTM);

        AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);
        const uint32 numVertices = mesh->GetNumVertices();

        m_auxVertices.clear();
        m_auxVertices.reserve(numVertices * 2);
        m_auxColors.clear();
        m_auxColors.reserve(m_auxVertices.size());

        // Render the tangents and bitangents
        AZ::Vector3 orgTangent, tangent, bitangent;
        for (uint32 i = 0; i < numVertices; ++i)
        {
            orgTangent.Set(tangents[i].GetX(), tangents[i].GetY(), tangents[i].GetZ());
            tangent = (worldTM.TransformVector(orgTangent)).GetNormalized();

            if (bitangents)
            {
                bitangent = bitangents[i];
            }
            else
            {
                bitangent = tangents[i].GetW() * normals[i].Cross(orgTangent);
            }
            bitangent = (worldTM.TransformVector(bitangent)).GetNormalizedSafe();

            m_auxVertices.emplace_back(m_worldSpacePositions[i]);
            m_auxColors.emplace_back(colorTangents);
            m_auxVertices.emplace_back(m_worldSpacePositions[i] + (tangent * scale));
            m_auxColors.emplace_back(colorTangents);

            if (tangents[i].GetW() < 0.0f)
            {
                m_auxVertices.emplace_back(m_worldSpacePositions[i]);
                m_auxColors.emplace_back(mirroredBitangentColor);
                m_auxVertices.emplace_back(m_worldSpacePositions[i] + (bitangent * scale));
                m_auxColors.emplace_back(mirroredBitangentColor);
            }
            else
            {
                m_auxVertices.emplace_back(m_worldSpacePositions[i]);
                m_auxColors.emplace_back(colorBitangents);
                m_auxVertices.emplace_back(m_worldSpacePositions[i] + (bitangent * scale));
                m_auxColors.emplace_back(colorBitangents);
            }
        }

        RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
        lineArgs.m_verts = m_auxVertices.data();
        lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
        lineArgs.m_colors = m_auxColors.data();
        lineArgs.m_colorCount = static_cast<uint32_t>(m_auxColors.size());
        lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
        auxGeom->DrawLines(lineArgs);
    }

    void AtomActorDebugDraw::RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM)
    {
        // Check if the mesh is valid and skip the node in case it's not
        if (!mesh)
        {
            return;
        }

        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();
        if (!auxGeom)
        {
            return;
        }

        PrepareForMesh(mesh, worldTM);

        const float scale = 0.01f;

        const AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);
        const AZ::Color vertexColor = AZ::Color(0.8f, 0.24f, 0.88f, 1.0f);

        const size_t numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
            const uint32 numTriangles = subMesh->GetNumPolygons();
            const uint32 startVertex = subMesh->GetStartVertex();
            const uint32* indices = subMesh->GetIndices();

            m_auxVertices.clear();
            m_auxVertices.reserve(numTriangles * 6);

            for (uint32 triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                const uint32 triangleStartIndex = triangleIndex * 3;
                const uint32 indexA = indices[triangleStartIndex + 0] + startVertex;
                const uint32 indexB = indices[triangleStartIndex + 1] + startVertex;
                const uint32 indexC = indices[triangleStartIndex + 2] + startVertex;

                const AZ::Vector3 posA = m_worldSpacePositions[indexA] + normals[indexA] * scale;
                const AZ::Vector3 posB = m_worldSpacePositions[indexB] + normals[indexB] * scale;
                const AZ::Vector3 posC = m_worldSpacePositions[indexC] + normals[indexC] * scale;

                m_auxVertices.emplace_back(posA);
                m_auxVertices.emplace_back(posB);

                m_auxVertices.emplace_back(posB);
                m_auxVertices.emplace_back(posC);

                m_auxVertices.emplace_back(posC);
                m_auxVertices.emplace_back(posA);
            }

            RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
            lineArgs.m_verts = m_auxVertices.data();
            lineArgs.m_vertCount = static_cast<uint32_t>(m_auxVertices.size());
            lineArgs.m_colors = &vertexColor;
            lineArgs.m_colorCount = 1;
            lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
            auxGeom->DrawLines(lineArgs);
        }
    }
} // namespace AZ::Render
