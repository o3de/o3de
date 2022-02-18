/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <AtomActorDebugDraw.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Integration/Rendering/RenderActorInstance.h>
#include <Integration/Rendering/RenderActorSettings.h>

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/RagdollInstance.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/JointSelectionBus.h>

namespace AZ::Render
{
    AtomActorDebugDraw::AtomActorDebugDraw(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
        m_auxGeomFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<RPI::AuxGeomFeatureProcessorInterface>(entityId);
    }

    void AtomActorDebugDraw::DebugDraw(const EMotionFX::ActorRenderFlags& renderFlags, EMotionFX::ActorInstance* instance)
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

        using AZ::RHI::CheckBitsAny;

        // Update the mesh deformers (perform cpu skinning and morphing) when needed.
        if (CheckBitsAny(renderFlags,
                EMotionFX::ActorRenderFlags::AABB | EMotionFX::ActorRenderFlags::FaceNormals | EMotionFX::ActorRenderFlags::Tangents |
                EMotionFX::ActorRenderFlags::VertexNormals | EMotionFX::ActorRenderFlags::Wireframe))
        {
            instance->UpdateMeshDeformers(0.0f, true);
        }

        const RPI::Scene* scene = RPI::Scene::GetSceneForEntityId(m_entityId);
        const RPI::ViewportContextPtr viewport = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetViewportContextByScene(scene);
        AzFramework::DebugDisplayRequests* debugDisplay = GetDebugDisplay(viewport->GetId());
        const AZ::Render::RenderActorSettings& renderActorSettings = EMotionFX::GetRenderActorSettings();
        const float scaleMultiplier = CalculateScaleMultiplier(instance);

        // Render aabb
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::AABB))
        {
            RenderAABB(instance,
                renderActorSettings.m_enabledNodeBasedAabb, renderActorSettings.m_nodeAABBColor,
                renderActorSettings.m_enabledMeshBasedAabb, renderActorSettings.m_meshAABBColor,
                renderActorSettings.m_enabledStaticBasedAabb, renderActorSettings.m_staticAABBColor);
        }

        // Render simple line skeleton
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::LineSkeleton))
        {
            RenderLineSkeleton(debugDisplay, instance, renderActorSettings.m_lineSkeletonColor);
        }

        // Render advanced skeleton
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::Skeleton))
        {
            RenderSkeleton(debugDisplay, instance, renderActorSettings.m_skeletonColor);
        }

        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::NodeNames))
        {
            RenderJointNames(instance, viewport, renderActorSettings.m_jointNameColor);
        }

        // Render internal EMFX debug lines.
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::EmfxDebug))
        {
            RenderEMFXDebugDraw(instance);
        }

        // Render
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::NodeOrientation))
        {
            RenderNodeOrientations(instance, debugDisplay, renderActorSettings.m_nodeOrientationScale * scaleMultiplier);
        }

        // Render vertex normal, face normal, tagent and wireframe.
        const bool renderVertexNormals = CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::VertexNormals);
        const bool renderFaceNormals = CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::FaceNormals);
        const bool renderTangents = CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::Tangents);
        const bool renderWireframe = CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::Wireframe);

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

                RenderNormals(mesh, globalTM, renderVertexNormals, renderFaceNormals, renderActorSettings.m_vertexNormalsScale,
                    renderActorSettings.m_faceNormalsScale, scaleMultiplier, renderActorSettings.m_vertexNormalsColor, renderActorSettings.m_faceNormalsColor);
                if (renderTangents)
                {
                    RenderTangents(mesh, globalTM, renderActorSettings.m_tangentsScale, scaleMultiplier,
                        renderActorSettings.m_tangentsColor, renderActorSettings.m_mirroredBitangentsColor, renderActorSettings.m_bitangentsColor);
                }
                if (renderWireframe)
                {
                    RenderWireframe(mesh, globalTM, renderActorSettings.m_wireframeScale * scaleMultiplier, renderActorSettings.m_wireframeColor);
                }
            }
        }

        // Hit detection colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::HitDetectionColliders))
        {
            RenderColliders(debugDisplay, EMotionFX::PhysicsSetup::HitDetection, instance,
                renderActorSettings.m_hitDetectionColliderColor, renderActorSettings.m_selectedHitDetectionColliderColor);
        }

        // Cloth colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::ClothColliders))
        {
            RenderColliders(debugDisplay, EMotionFX::PhysicsSetup::Cloth, instance,
                renderActorSettings.m_clothColliderColor, renderActorSettings.m_selectedClothColliderColor);
        }

        // Simulated object colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::SimulatedObjectColliders))
        {
            RenderColliders(debugDisplay, EMotionFX::PhysicsSetup::SimulatedObjectCollider, instance,
                renderActorSettings.m_simulatedObjectColliderColor, renderActorSettings.m_selectedSimulatedObjectColliderColor);
        }

        // Ragdoll
        const bool renderRagdollColliders = AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::RagdollColliders);
        const bool renderRagdollJointLimits = AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::RagdollJointLimits);
        if (renderRagdollColliders || renderRagdollJointLimits)
        {
            RenderRagdoll(debugDisplay, instance, renderRagdollColliders, renderRagdollJointLimits);
        }
    }

    float AtomActorDebugDraw::CalculateScaleMultiplier(EMotionFX::ActorInstance* instance) const
    {
        const AZ::Aabb aabb = instance->GetAabb();
        const float aabbRadius = aabb.GetExtents().GetLength() * 0.5f;
        // Scale the multiplier down to 1% of the character size, that looks pretty nice on most of the models.
        return aabbRadius * 0.01f;
    }

    float AtomActorDebugDraw::CalculateBoneScale(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        // Get the transform data
        EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();

        const size_t nodeIndex = node->GetNodeIndex();
        const size_t parentIndex = node->GetParentIndex();
        const AZ::Vector3 nodeWorldPos = pose->GetWorldSpaceTransform(nodeIndex).m_position;

        if (parentIndex != InvalidIndex)
        {
            const AZ::Vector3 parentWorldPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
            const AZ::Vector3 bone = parentWorldPos - nodeWorldPos;
            const float boneLength = bone.GetLengthEstimate();

            // 10% of the bone length is the sphere size
            return boneLength * 0.1f;
        }

        return 0.0f;
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

    AzFramework::DebugDisplayRequests* AtomActorDebugDraw::GetDebugDisplay(AzFramework::ViewportId viewportId)
    {
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, viewportId);
        return AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
    }

    void AtomActorDebugDraw::RenderAABB(EMotionFX::ActorInstance* instance,
            bool enableNodeAabb,
            const AZ::Color& nodeAabbColor,
            bool enableMeshAabb,
            const AZ::Color& meshAabbColor,
            bool enableStaticAabb,
            const AZ::Color& staticAabbColor)
    {
        RPI::AuxGeomDrawPtr auxGeom = m_auxGeomFeatureProcessor->GetDrawQueue();

        if (enableNodeAabb)
        {
            AZ::Aabb aabb;
            instance->CalcNodeBasedAabb(&aabb);
            if (aabb.IsValid())
            {
                auxGeom->DrawAabb(aabb, nodeAabbColor, RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }

        if (enableMeshAabb)
        {
            AZ::Aabb aabb;
            const size_t lodLevel = instance->GetLODLevel();
            instance->CalcMeshBasedAabb(lodLevel, &aabb);
            if (aabb.IsValid())
            {
                auxGeom->DrawAabb(aabb, meshAabbColor, RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }

        if (enableStaticAabb)
        {
            AZ::Aabb aabb;
            instance->CalcStaticBasedAabb(&aabb);
            if (aabb.IsValid())
            {
                auxGeom->DrawAabb(aabb, staticAabbColor, RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }
    }

    void AtomActorDebugDraw::RenderLineSkeleton(AzFramework::DebugDisplayRequests* debugDisplay, EMotionFX::ActorInstance* instance, const AZ::Color& color) const
    {
        const EMotionFX::TransformData* transformData = instance->GetTransformData();
        const EMotionFX::Skeleton* skeleton = instance->GetActor()->GetSkeleton();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const size_t lodLevel = instance->GetLODLevel();
        const size_t numJoints = skeleton->GetNumNodes();

        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, instance);

        const AZ::u32 oldState = debugDisplay->GetState();
        debugDisplay->DepthTestOff();

        AZ::Color renderColor;
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

            if (cachedSelectedJointIndices && cachedSelectedJointIndices->find(jointIndex) != cachedSelectedJointIndices->end())
            {
                renderColor = SelectedColor;
            }
            else
            {
                renderColor = color;
            }

            const AZ::Vector3 parentPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
            const AZ::Vector3 bonePos = pose->GetWorldSpaceTransform(jointIndex).m_position;

            debugDisplay->SetColor(renderColor);
            debugDisplay->DrawLine(parentPos, bonePos);
        }

        debugDisplay->SetState(oldState);
    }

    void AtomActorDebugDraw::RenderSkeleton(AzFramework::DebugDisplayRequests* debugDisplay, EMotionFX::ActorInstance* instance, const AZ::Color& color)
    {
        const EMotionFX::TransformData* transformData = instance->GetTransformData();
        const EMotionFX::Skeleton* skeleton = instance->GetActor()->GetSkeleton();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const size_t numEnabled = instance->GetNumEnabledNodes();

        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, instance);

        const AZ::u32 oldState = debugDisplay->GetState();
        debugDisplay->DepthTestOff();

        AZ::Color renderColor;
        for (size_t i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node* joint = skeleton->GetNode(instance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();
            const size_t parentIndex = joint->GetParentIndex();

            // check if this node has a parent and is a bone, if not skip it
            if (parentIndex == InvalidIndex)
            {
                continue;
            }

            const AZ::Vector3 nodeWorldPos = pose->GetWorldSpaceTransform(jointIndex).m_position;
            const AZ::Vector3 parentWorldPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
            const AZ::Vector3 bone = parentWorldPos - nodeWorldPos;
            const AZ::Vector3 boneDirection = bone.GetNormalizedEstimate();
            const AZ::Vector3 centerWorldPos = bone / 2 + nodeWorldPos;
            const float boneLength = bone.GetLengthEstimate();
            const float boneScale = CalculateBoneScale(instance, joint);
            const float parentBoneScale = CalculateBoneScale(instance, skeleton->GetNode(parentIndex));
            const float cylinderSize = boneLength - boneScale - parentBoneScale;

            if (cachedSelectedJointIndices && cachedSelectedJointIndices->find(jointIndex) != cachedSelectedJointIndices->end())
            {
                renderColor = SelectedColor;
            }
            else
            {
                renderColor = color;
            }
            renderColor.SetA(0.75f);
            debugDisplay->SetColor(renderColor);

            // Render the bone cylinder, the cylinder will be directed towards the node's parent and must fit between the spheres
            debugDisplay->DrawSolidCylinder(centerWorldPos, boneDirection, boneScale * 0.75f, cylinderSize);
            debugDisplay->DrawBall(nodeWorldPos, boneScale);
        }

        debugDisplay->SetState(oldState);
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
        lineArgs.m_vertCount = aznumeric_cast<uint32_t>(m_auxVertices.size());
        lineArgs.m_colors = m_auxColors.data();
        lineArgs.m_colorCount = aznumeric_cast<uint32_t>(m_auxColors.size());
        lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::Off;
        auxGeom->DrawLines(lineArgs);
    }

    void AtomActorDebugDraw::RenderNormals(
        EMotionFX::Mesh* mesh,
        const AZ::Transform& worldTM,
        bool vertexNormals,
        bool faceNormals,
        float vertexNormalsScale,
        float faceNormalsScale,
        float scaleMultiplier,
        const AZ::Color& vertexNormalsColor,
        const AZ::Color& faceNormalsColor)
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
                    m_auxVertices.emplace_back(normalPos + (normalDir * faceNormalsScale * scaleMultiplier));
                }

                RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
                lineArgs.m_verts = m_auxVertices.data();
                lineArgs.m_vertCount = aznumeric_cast<uint32_t>(m_auxVertices.size());
                lineArgs.m_colors = &faceNormalsColor;
                lineArgs.m_colorCount = 1;
                lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::On;
                auxGeom->DrawLines(lineArgs);
            }
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
                    const AZ::Vector3 normal = worldTM.TransformVector(normals[vertexIndex]).GetNormalizedSafe() *
                        vertexNormalsScale * scaleMultiplier;

                    m_auxVertices.emplace_back(position);
                    m_auxVertices.emplace_back(position + normal);
                }

                RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
                lineArgs.m_verts = m_auxVertices.data();
                lineArgs.m_vertCount = aznumeric_cast<uint32_t>(m_auxVertices.size());
                lineArgs.m_colors = &vertexNormalsColor;
                lineArgs.m_colorCount = 1;
                lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::On;
                auxGeom->DrawLines(lineArgs);
            }
        }
    }

    void AtomActorDebugDraw::RenderTangents(
        EMotionFX::Mesh* mesh,
        const AZ::Transform& worldTM,
        float tangentsScale,
        float scaleMultiplier,
        const AZ::Color& tangentsColor,
        const AZ::Color& mirroredBitangentsColor,
        const AZ::Color& bitangentsColor)
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
            m_auxColors.emplace_back(tangentsColor);
            m_auxVertices.emplace_back(m_worldSpacePositions[i] + (tangent * tangentsScale * scaleMultiplier));
            m_auxColors.emplace_back(tangentsColor);

            if (tangents[i].GetW() < 0.0f)
            {
                m_auxVertices.emplace_back(m_worldSpacePositions[i]);
                m_auxColors.emplace_back(mirroredBitangentsColor);
                m_auxVertices.emplace_back(m_worldSpacePositions[i] + (bitangent * tangentsScale * scaleMultiplier));
                m_auxColors.emplace_back(mirroredBitangentsColor);
            }
            else
            {
                m_auxVertices.emplace_back(m_worldSpacePositions[i]);
                m_auxColors.emplace_back(bitangentsColor);
                m_auxVertices.emplace_back(m_worldSpacePositions[i] + (bitangent * tangentsScale * scaleMultiplier));
                m_auxColors.emplace_back(bitangentsColor);
            }
        }

        RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
        lineArgs.m_verts = m_auxVertices.data();
        lineArgs.m_vertCount = aznumeric_cast<uint32_t>(m_auxVertices.size());
        lineArgs.m_colors = m_auxColors.data();
        lineArgs.m_colorCount = aznumeric_cast<uint32_t>(m_auxColors.size());
        lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::On;
        auxGeom->DrawLines(lineArgs);
    }

    void AtomActorDebugDraw::RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM,
        float scale, const AZ::Color& color)
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
        const AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);

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
            lineArgs.m_vertCount = aznumeric_cast<uint32_t>(m_auxVertices.size());
            lineArgs.m_colors = &color;
            lineArgs.m_colorCount = 1;
            lineArgs.m_depthTest = RPI::AuxGeomDraw::DepthTest::On;
            auxGeom->DrawLines(lineArgs);
        }
    }

    void AtomActorDebugDraw::RenderJointNames(EMotionFX::ActorInstance* actorInstance,
        RPI::ViewportContextPtr viewportContext, const AZ::Color& jointNameColor)
    {
        if (!m_fontDrawInterface)
        {
            auto fontQueryInterface = AZ::Interface<AzFramework::FontQueryInterface>::Get();
            if (!fontQueryInterface)
            {
                return;
            }
            m_fontDrawInterface = fontQueryInterface->GetDefaultFontDrawInterface();
        }

        if (!m_fontDrawInterface || !viewportContext || !viewportContext->GetRenderScene() ||
            !AZ::Interface<AzFramework::FontQueryInterface>::Get())
        {
            return;
        }

        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, actorInstance);

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const size_t numEnabledNodes = actorInstance->GetNumEnabledNodes();

        m_drawParams.m_drawViewportId = viewportContext->GetId();
        AzFramework::WindowSize viewportSize = viewportContext->GetViewportSize();
        m_drawParams.m_position = AZ::Vector3(aznumeric_cast<float>(viewportSize.m_width), 0.0f, 1.0f) +
            TopRightBorderPadding * viewportContext->GetDpiScalingFactor();
        m_drawParams.m_scale = AZ::Vector2(BaseFontSize);
        m_drawParams.m_hAlign = AzFramework::TextHorizontalAlignment::Right;
        m_drawParams.m_monospace = false;
        m_drawParams.m_depthTest = false;
        m_drawParams.m_virtual800x600ScreenSize = false;
        m_drawParams.m_scaleWithWindow = false;
        m_drawParams.m_multiline = true;
        m_drawParams.m_lineSpacing = 0.5f;

        for (size_t i = 0; i < numEnabledNodes; ++i)
        {
            const EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();
            const AZ::Vector3 worldPos = pose->GetWorldSpaceTransform(jointIndex).m_position;

            m_drawParams.m_position = worldPos;
            if (cachedSelectedJointIndices && cachedSelectedJointIndices->find(jointIndex) != cachedSelectedJointIndices->end())
            {
                m_drawParams.m_color = SelectedColor;
            }
            else
            {
                m_drawParams.m_color = jointNameColor;
            }
            m_fontDrawInterface->DrawScreenAlignedText3d(m_drawParams, joint->GetName());
        }
    }

    void AtomActorDebugDraw::RenderNodeOrientations(EMotionFX::ActorInstance* actorInstance,
        AzFramework::DebugDisplayRequests* debugDisplay, float scale)
    {
        // Get the actor and the transform data
        const float unitScale =
            1.0f / (float)MCore::Distance::ConvertValue(1.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const float constPreScale = scale * unitScale * 3.0f;

        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, actorInstance);

        const size_t numEnabled = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();

            static const float axisBoneScale = 50.0f;
            const float size = CalculateBoneScale(actorInstance, joint) * constPreScale * axisBoneScale;
            AZ::Transform worldTM = pose->GetWorldSpaceTransform(jointIndex).ToAZTransform();
            bool selected = false;
            if (cachedSelectedJointIndices && cachedSelectedJointIndices->find(jointIndex) != cachedSelectedJointIndices->end())
            {
                selected = true;
            }
            RenderLineAxis(debugDisplay, worldTM, size, selected);
        }
    }

    void AtomActorDebugDraw::RenderLineAxis(
        AzFramework::DebugDisplayRequests* debugDisplay,
        AZ::Transform worldTM,
        float size,
        bool selected,
        bool renderAxisName)
    {
        const float axisHeight = size * 0.7f;
        const float frontSize = size * 5.0f + 0.2f;
        const AZ::Vector3 position = worldTM.GetTranslation();

        // Render x axis
        {
            AZ::Color xSelectedColor = selected ? AZ::Colors::Orange : AZ::Colors::Red;

            const AZ::Vector3 xAxisDir = (worldTM.TransformPoint(AZ::Vector3(size, 0.0f, 0.0f)) - position).GetNormalized();
            const AZ::Vector3 xAxisArrowStart = position + xAxisDir * axisHeight;
            debugDisplay->SetColor(xSelectedColor);
            debugDisplay->DrawArrow(position, xAxisArrowStart, size);

            if (renderAxisName)
            {
                const AZ::Vector3 xNamePos = position + xAxisDir * (size * 1.15f);
                debugDisplay->DrawTextLabel(xNamePos, frontSize, "X");
            }
        }

        // Render y axis
        {
            AZ::Color ySelectedColor = selected ? AZ::Colors::Orange : AZ::Colors::Blue;

            const AZ::Vector3 yAxisDir = (worldTM.TransformPoint(AZ::Vector3(0.0f, size, 0.0f)) - position).GetNormalized();
            const AZ::Vector3 yAxisArrowStart = position + yAxisDir * axisHeight;
            debugDisplay->SetColor(ySelectedColor);
            debugDisplay->DrawArrow(position, yAxisArrowStart, size);

            if (renderAxisName)
            {
                const AZ::Vector3 yNamePos = position + yAxisDir * (size * 1.15f);
                debugDisplay->DrawTextLabel(yNamePos, frontSize, "Y");
            }
        }

        // Render z axis
        {
            AZ::Color zSelectedColor = selected ? AZ::Colors::Orange : AZ::Colors::Green;

            const AZ::Vector3 zAxisDir = (worldTM.TransformPoint(AZ::Vector3(0.0f, 0.0f, size)) - position).GetNormalized();
            const AZ::Vector3 zAxisArrowStart = position + zAxisDir * axisHeight;
            debugDisplay->SetColor(zSelectedColor);
            debugDisplay->DrawArrow(position, zAxisArrowStart, size);

            if (renderAxisName)
            {
                const AZ::Vector3 zNamePos = position + zAxisDir * (size * 1.15f);
                debugDisplay->DrawTextLabel(zNamePos, frontSize, "Z");
            }
        }
    }

    void AtomActorDebugDraw::RenderColliders(AzFramework::DebugDisplayRequests* debugDisplay,
        const AzPhysics::ShapeColliderPairList& colliders,
        const EMotionFX::ActorInstance* actorInstance,
        const EMotionFX::Node* node,
        const AZ::Color& colliderColor) const
    {
        if (!debugDisplay)
        {
            return;
        }

        const size_t nodeIndex = node->GetNodeIndex();

        for (const auto& collider : colliders)
        {
#ifndef EMFX_SCALE_DISABLED
            const AZ::Vector3& worldScale = actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex).m_scale;
#else
            const AZ::Vector3 worldScale = AZ::Vector3::CreateOne();
#endif

            const EMotionFX::Transform colliderOffsetTransform(collider.first->m_position, collider.first->m_rotation);
            const EMotionFX::Transform& actorInstanceGlobalTransform = actorInstance->GetWorldSpaceTransform();
            const EMotionFX::Transform& emfxNodeGlobalTransform =
                actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex);
            const EMotionFX::Transform emfxColliderGlobalTransformNoScale =
                colliderOffsetTransform * emfxNodeGlobalTransform * actorInstanceGlobalTransform;

            const AZ::TypeId colliderType = collider.second->RTTI_GetType();
            if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
            {
                Physics::SphereShapeConfiguration* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());

                // O3DE Physics scaling rules: The maximum component from the node scale will be multiplied by the radius of the sphere.
                const float radius = sphere->m_radius *
                    MCore::Max3<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()),
                        static_cast<float>(worldScale.GetZ()));

                debugDisplay->DepthTestOff();
                debugDisplay->SetColor(colliderColor);
                debugDisplay->DrawWireSphere(emfxColliderGlobalTransformNoScale.m_position, radius);
            }
            else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
            {
                Physics::CapsuleShapeConfiguration* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());

                // O3DE Physics scaling rules: The maximum of the X/Y scale components of the node scale will be multiplied by the radius of
                // the capsule. The Z component of the entity scale will be multiplied by the height of the capsule.
                const float radius =
                    capsule->m_radius * MCore::Max<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()));
                const float height = capsule->m_height * static_cast<float>(worldScale.GetZ());

                debugDisplay->DepthTestOff();
                debugDisplay->SetColor(colliderColor);
                debugDisplay->DrawWireCapsule(
                    emfxColliderGlobalTransformNoScale.m_position, emfxColliderGlobalTransformNoScale.ToAZTransform().GetBasisZ(), radius, height);
            }
            else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
            {
                Physics::BoxShapeConfiguration* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());

                // O3DE Physics scaling rules: Each component of the box dimensions will be scaled by the node's world scale.
                AZ::Vector3 dimensions = box->m_dimensions;
                dimensions *= worldScale;

                debugDisplay->DepthTestOff();
                debugDisplay->SetColor(colliderColor);
                debugDisplay->DrawWireBox(
                    emfxColliderGlobalTransformNoScale.m_position, emfxColliderGlobalTransformNoScale.m_position + dimensions);
            }
        }
    }

    void AtomActorDebugDraw::RenderColliders(AzFramework::DebugDisplayRequests* debugDisplay,
        EMotionFX::PhysicsSetup::ColliderConfigType colliderConfigType,
        EMotionFX::ActorInstance* actorInstance,
        const AZ::Color& defaultColor,
        const AZ::Color& selectedColor) const
    {
        if (colliderConfigType == EMotionFX::PhysicsSetup::Unknown)
        {
            return;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(colliderConfigType);

        if (colliderConfig)
        {
            const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
            EMotionFX::JointSelectionRequestBus::BroadcastResult(
                cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, actorInstance);

            for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
            {
                const EMotionFX::Node* joint = actor->GetSkeleton()->FindNodeByName(nodeConfig.m_name.c_str());
                if (joint)
                {
                    const bool jointSelected = cachedSelectedJointIndices &&
                        (cachedSelectedJointIndices->empty() || cachedSelectedJointIndices->find(joint->GetNodeIndex()) != cachedSelectedJointIndices->end());

                    const AzPhysics::ShapeColliderPairList& colliders = nodeConfig.m_shapes;
                    RenderColliders(debugDisplay, colliders, actorInstance, joint, jointSelected ? selectedColor : defaultColor);
                }
            }
        }
    }

    void AtomActorDebugDraw::RenderJointFrame(AzFramework::DebugDisplayRequests* debugDisplay,
        const AzPhysics::JointConfiguration& configuration,
        const EMotionFX::ActorInstance* actorInstance,
        const EMotionFX::Node* node,
        const AZ::Color& color) const
    {
        const EMotionFX::Transform& actorInstanceWorldSpaceTransform = actorInstance->GetWorldSpaceTransform();
        const EMotionFX::Pose* currentPose = actorInstance->GetTransformData()->GetCurrentPose();
        const EMotionFX::Transform childJointLocalSpaceTransform(AZ::Vector3::CreateZero(), configuration.m_childLocalRotation);
        const EMotionFX::Transform childModelSpaceTransform =
            childJointLocalSpaceTransform * currentPose->GetModelSpaceTransform(node->GetNodeIndex());
        const EMotionFX::Transform jointChildWorldSpaceTransformNoScale = (childModelSpaceTransform * actorInstanceWorldSpaceTransform);
        AZ::Vector3 dir = jointChildWorldSpaceTransformNoScale.ToAZTransform().GetBasisX();

        debugDisplay->SetColor(color);
        debugDisplay->DrawArrow(jointChildWorldSpaceTransformNoScale.m_position, jointChildWorldSpaceTransformNoScale.m_position + dir, 0.1f);
    }

    void AtomActorDebugDraw::JointLimitRenderData::Clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_lineBuffer.clear();
        m_lineValidityBuffer.clear();
    }

    void AtomActorDebugDraw::RenderJointLimit(AzFramework::DebugDisplayRequests* debugDisplay,
        const AzPhysics::JointConfiguration& configuration,
        const EMotionFX::ActorInstance* actorInstance,
        const EMotionFX::Node* node,
        const EMotionFX::Node* parentNode,
        const AZ::Color& regularColor,
        const AZ::Color& violatedColor)
    {
        const size_t nodeIndex = node->GetNodeIndex();
        const size_t parentNodeIndex = parentNode->GetNodeIndex();
        const EMotionFX::Transform& actorInstanceWorldTransform = actorInstance->GetWorldSpaceTransform();
        const EMotionFX::Pose* currentPose = actorInstance->GetTransformData()->GetCurrentPose();
        const AZ::Quaternion& parentOrientation = currentPose->GetModelSpaceTransform(parentNodeIndex).m_rotation;
        const AZ::Quaternion& childOrientation = currentPose->GetModelSpaceTransform(nodeIndex).m_rotation;

        m_jointLimitRenderData.Clear();
        if (auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
        {
            jointHelpers->GenerateJointLimitVisualizationData(
                configuration, parentOrientation, childOrientation, s_scale, s_angularSubdivisions, s_radialSubdivisions,
                m_jointLimitRenderData.m_vertexBuffer, m_jointLimitRenderData.m_indexBuffer,
                m_jointLimitRenderData.m_lineBuffer, m_jointLimitRenderData.m_lineValidityBuffer);
        }

        EMotionFX::Transform jointModelSpaceTransform = currentPose->GetModelSpaceTransform(parentNodeIndex);
        jointModelSpaceTransform.m_position = currentPose->GetModelSpaceTransform(nodeIndex).m_position;
        const EMotionFX::Transform jointGlobalTransformNoScale = jointModelSpaceTransform * actorInstanceWorldTransform;

        const size_t numLineBufferEntries = m_jointLimitRenderData.m_lineBuffer.size();
        if (m_jointLimitRenderData.m_lineValidityBuffer.size() * 2 != numLineBufferEntries)
        {
            AZ_ErrorOnce("EMotionFX", false, "Unexpected buffer size in joint limit visualization for node %s", node->GetName());
            return;
        }

        for (size_t i = 0; i < numLineBufferEntries; i += 2)
        {
            const AZ::Color& lineColor = m_jointLimitRenderData.m_lineValidityBuffer[i / 2] ? regularColor : violatedColor;
            debugDisplay->DepthTestOff();
            debugDisplay->DrawLine(
                jointGlobalTransformNoScale.TransformPoint(m_jointLimitRenderData.m_lineBuffer[i]),
                jointGlobalTransformNoScale.TransformPoint(m_jointLimitRenderData.m_lineBuffer[i + 1]), lineColor.GetAsVector4(), lineColor.GetAsVector4()
            );
        }
    }

    void AtomActorDebugDraw::RenderRagdoll(AzFramework::DebugDisplayRequests* debugDisplay,
        EMotionFX::ActorInstance* actorInstance,
        bool renderColliders,
        bool renderJointLimits)
    {
        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
        const AZStd::vector<Physics::RagdollNodeConfiguration>& ragdollNodes = ragdollConfig.m_nodes;
        const Physics::CharacterColliderConfiguration& colliderConfig = ragdollConfig.m_colliders;
        const EMotionFX::RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();

        const AZ::Render::RenderActorSettings& settings = EMotionFX::GetRenderActorSettings();
        const AZ::Color& violatedColor = settings.m_violatedJointLimitColor;
        const AZ::Color& defaultColor = settings.m_ragdollColliderColor;
        const AZ::Color& selectedColor = settings.m_selectedRagdollColliderColor;

        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, actorInstance);

        for (size_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            const EMotionFX::Node* joint = skeleton->GetNode(nodeIndex);
            const size_t jointIndex = joint->GetNodeIndex();

            AZ::Outcome<size_t> ragdollNodeIndex = AZ::Failure();
            if (ragdollInstance)
            {
                ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
            }
            else
            {
                ragdollNodeIndex = ragdollConfig.FindNodeConfigIndexByName(joint->GetNameString());
            }

            if (!ragdollNodeIndex.IsSuccess())
            {
                continue;
            }

            const bool jointSelected = cachedSelectedJointIndices &&
                (cachedSelectedJointIndices->empty() || cachedSelectedJointIndices->find(joint->GetNodeIndex()) != cachedSelectedJointIndices->end());

            AZ::Color finalColor;
            if (jointSelected)
            {
                finalColor = selectedColor;
            }
            else
            {
                finalColor = defaultColor;
            }

            const Physics::RagdollNodeConfiguration& ragdollNode = ragdollNodes[ragdollNodeIndex.GetValue()];

            if (renderColliders)
            {
                const Physics::CharacterColliderNodeConfiguration* colliderNodeConfig =
                    colliderConfig.FindNodeConfigByName(joint->GetNameString());
                if (colliderNodeConfig)
                {
                    const AzPhysics::ShapeColliderPairList& colliders = colliderNodeConfig->m_shapes;
                    RenderColliders(debugDisplay, colliders, actorInstance, joint, finalColor);
                }
            }

            if (renderJointLimits && jointSelected)
            {
                const AZStd::shared_ptr<AzPhysics::JointConfiguration>& jointLimitConfig = ragdollNode.m_jointConfig;
                if (jointLimitConfig)
                {
                    const EMotionFX::Node* ragdollParentNode = physicsSetup->FindRagdollParentNode(joint);
                    if (ragdollParentNode)
                    {
                        RenderJointLimit(debugDisplay, *jointLimitConfig, actorInstance, joint, ragdollParentNode, finalColor, violatedColor);
                        RenderJointFrame(debugDisplay, *jointLimitConfig, actorInstance, joint, finalColor);
                    }
                }
            }
        }
    }
} // namespace AZ::Render
