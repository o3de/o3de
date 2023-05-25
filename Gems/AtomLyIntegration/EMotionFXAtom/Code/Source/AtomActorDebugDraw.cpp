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
        AZ_Assert(m_auxGeomFeatureProcessor, "AuxGeomFeatureProcessor doesn't exist. Check if it is missing from AnimViewport.setreg file.");
    }

    // Function for providing data required for debug drawing colliders
    Physics::CharacterPhysicsDebugDraw::NodeDebugDrawData GetNodeDebugDrawData(
        const Physics::CharacterColliderNodeConfiguration& colliderNodeConfig,
        const EMotionFX::ActorInstance* instance,
        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices,
        size_t cachedHoveredJointIndex)
    {
        Physics::CharacterPhysicsDebugDraw::NodeDebugDrawData nodeDebugDrawData;
        const EMotionFX::Actor* actor = instance->GetActor();
        const EMotionFX::Node* joint = actor->GetSkeleton()->FindNodeByName(colliderNodeConfig.m_name.c_str());
        if (!joint)
        {
            nodeDebugDrawData.m_valid = false;
            return nodeDebugDrawData;
        }

        const size_t nodeIndex = joint->GetNodeIndex();
        nodeDebugDrawData.m_selected = cachedSelectedJointIndices &&
            (cachedSelectedJointIndices->empty() || cachedSelectedJointIndices->find(nodeIndex) != cachedSelectedJointIndices->end());
        nodeDebugDrawData.m_hovered = (nodeIndex == cachedHoveredJointIndex);

        const EMotionFX::Transform& actorInstanceGlobalTransform = instance->GetWorldSpaceTransform();
        const EMotionFX::Transform& emfxNodeGlobalTransform =
            instance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex);
        nodeDebugDrawData.m_worldTransform = (emfxNodeGlobalTransform * actorInstanceGlobalTransform).ToAZTransform();
        nodeDebugDrawData.m_valid = true;
        return nodeDebugDrawData;
    };

    // Function for proviuding data required for debug drawing joint limits
    Physics::CharacterPhysicsDebugDraw::JointDebugDrawData GetJointDebugDrawData(
        const Physics::RagdollNodeConfiguration& ragdollNodeConfig,
        const EMotionFX::ActorInstance* instance,
        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices,
        [[maybe_unused]] size_t cachedHoveredJointIndex)
    {
        Physics::CharacterPhysicsDebugDraw::JointDebugDrawData jointDebugDrawData;
        const EMotionFX::Actor* actor = instance->GetActor();
        const EMotionFX::Node* joint = actor->GetSkeleton()->FindNodeByName(ragdollNodeConfig.m_debugName.c_str());
        if (!joint)
        {
            jointDebugDrawData.m_valid = false;
            return jointDebugDrawData;
        }

        jointDebugDrawData.m_valid = true;
        const size_t nodeIndex = joint->GetNodeIndex();
        const bool jointSelected = cachedSelectedJointIndices &&
            (cachedSelectedJointIndices->empty() || cachedSelectedJointIndices->find(nodeIndex) != cachedSelectedJointIndices->end());

        if (!jointSelected)
        {
            jointDebugDrawData.m_visible = false;
            return jointDebugDrawData;
        }

        const EMotionFX::Node* ragdollParentNode = instance->GetActor()->GetPhysicsSetup()->FindRagdollParentNode(joint);
        if (!ragdollParentNode)
        {
            jointDebugDrawData.m_valid = false;
            return jointDebugDrawData;
        }

        const size_t ragdollParentNodeIndex = ragdollParentNode->GetNodeIndex();
        const EMotionFX::Pose* currentPose = instance->GetTransformData()->GetCurrentPose();
        const EMotionFX::Transform& childModelSpaceTransform = currentPose->GetModelSpaceTransform(nodeIndex);
        jointDebugDrawData.m_childModelSpaceOrientation = childModelSpaceTransform.m_rotation;
        jointDebugDrawData.m_parentModelSpaceOrientation = currentPose->GetModelSpaceTransform(ragdollParentNodeIndex).m_rotation;
        EMotionFX::Transform parentModelSpaceTransform = currentPose->GetModelSpaceTransform(ragdollParentNodeIndex);
        parentModelSpaceTransform.m_position = currentPose->GetModelSpaceTransform(nodeIndex).m_position;
        jointDebugDrawData.m_parentWorldTransform = (parentModelSpaceTransform * instance->GetWorldSpaceTransform()).ToAZTransform();
        jointDebugDrawData.m_childWorldTransform = (childModelSpaceTransform * instance->GetWorldSpaceTransform()).ToAZTransform();
        jointDebugDrawData.m_visible = true;
        jointDebugDrawData.m_selected = true;
        return jointDebugDrawData;
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

        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::NodeOrientation))
        {
            RenderNodeOrientations(instance, debugDisplay, renderActorSettings.m_nodeOrientationScale * scaleMultiplier);
        }

        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::MotionExtraction))
        {
            RenderTrajectoryPath(debugDisplay, instance, renderActorSettings.m_trajectoryHeadColor, renderActorSettings.m_trajectoryPathColor);
        }

        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::RootMotion))
        {
            RenderRootMotion(debugDisplay, instance, AZ::Colors::Red);
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
                const AZ::Transform globalTM = pose->GetMeshNodeWorldSpaceTransform(geomLODLevel, node->GetNodeIndex()).ToAZTransform();

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

        // Data required for debug drawing colliders and ragdolls
        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedSelectedJointIndices, &EMotionFX::JointSelectionRequests::FindSelectedJointIndices, instance);

        size_t cachedHoveredJointIndex = InvalidIndex;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedHoveredJointIndex, &EMotionFX::JointSelectionRequests::FindHoveredJointIndex, instance);

        Physics::CharacterPhysicsDebugDraw::NodeDebugDrawDataFunction nodeDebugDrawDataFunction =
            [instance, cachedSelectedJointIndices, cachedHoveredJointIndex](
                const Physics::CharacterColliderNodeConfiguration& colliderNodeConfig)
        {
            return GetNodeDebugDrawData(colliderNodeConfig, instance, cachedSelectedJointIndices, cachedHoveredJointIndex);
        };

        Physics::CharacterPhysicsDebugDraw::JointDebugDrawDataFunction jointDebugDrawDataFunction =
            [instance, cachedSelectedJointIndices, cachedHoveredJointIndex](const Physics::RagdollNodeConfiguration& ragdollNodeConfig)
        {
            return GetJointDebugDrawData(ragdollNodeConfig, instance, cachedSelectedJointIndices, cachedHoveredJointIndex);
        };

        // Hit detection colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::HitDetectionColliders))
        {
            m_characterPhysicsDebugDraw.RenderColliders(
                debugDisplay,
                instance->GetActor()->GetPhysicsSetup()->GetColliderConfigByType(EMotionFX::PhysicsSetup::HitDetection),
                nodeDebugDrawDataFunction,
                Physics::CharacterPhysicsDebugDraw::ColorSettings{ renderActorSettings.m_hitDetectionColliderColor,
                                                                   renderActorSettings.m_selectedHitDetectionColliderColor,
                                                                   renderActorSettings.m_hoveredHitDetectionColliderColor });
        }

        // Cloth colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::ClothColliders))
        {
            m_characterPhysicsDebugDraw.RenderColliders(
                debugDisplay,
                instance->GetActor()->GetPhysicsSetup()->GetColliderConfigByType(EMotionFX::PhysicsSetup::Cloth),
                nodeDebugDrawDataFunction,
                Physics::CharacterPhysicsDebugDraw::ColorSettings{ renderActorSettings.m_clothColliderColor,
                                                                   renderActorSettings.m_selectedClothColliderColor,
                                                                   renderActorSettings.m_hoveredClothColliderColor });
        }

        // Simulated object colliders
        if (CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::SimulatedObjectColliders))
        {
            m_characterPhysicsDebugDraw.RenderColliders(
                debugDisplay,
                instance->GetActor()->GetPhysicsSetup()->GetColliderConfigByType(EMotionFX::PhysicsSetup::SimulatedObjectCollider),
                nodeDebugDrawDataFunction,
                Physics::CharacterPhysicsDebugDraw::ColorSettings{ renderActorSettings.m_simulatedObjectColliderColor,
                                                                   renderActorSettings.m_selectedSimulatedObjectColliderColor,
                                                                   renderActorSettings.m_hoveredSimulatedObjectColliderColor });
        }

        // Ragdoll
        if (AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::RagdollColliders))
        {
            Physics::CharacterColliderConfiguration* ragdollColliderConfiguration =
                instance->GetActor()->GetPhysicsSetup()->GetColliderConfigByType(EMotionFX::PhysicsSetup::Ragdoll);
            Physics::ParentIndices parentIndices;
            parentIndices.reserve(ragdollColliderConfiguration->m_nodes.size());

            for (const auto& nodeConfiguration : ragdollColliderConfiguration->m_nodes)
            {
                AZ::Outcome<size_t> parentIndexOutcome;
                const EMotionFX::Skeleton* skeleton = instance->GetActor()->GetSkeleton();
                EMotionFX::Node* childNode = skeleton->FindNodeByName(nodeConfiguration.m_name);
                if (childNode)
                {
                    const EMotionFX::Node* parentNode = childNode->GetParentNode();
                    if (parentNode)
                    {
                        parentIndexOutcome = ragdollColliderConfiguration->FindNodeConfigIndexByName(parentNode->GetNameString());
                    }
                }
                parentIndices.push_back(parentIndexOutcome.GetValueOr(SIZE_MAX));
            }

            m_characterPhysicsDebugDraw.RenderRagdollColliders(
                debugDisplay,
                ragdollColliderConfiguration,
                nodeDebugDrawDataFunction,
                parentIndices,
                Physics::CharacterPhysicsDebugDraw::ColorSettings{ renderActorSettings.m_ragdollColliderColor,
                                                                   renderActorSettings.m_selectedRagdollColliderColor,
                                                                   renderActorSettings.m_hoveredRagdollColliderColor,
                                                                   renderActorSettings.m_violatedRagdollColliderColor });
        }
        if (AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::RagdollJointLimits))
        {
            m_characterPhysicsDebugDraw.RenderJointLimits(
                debugDisplay,
                instance->GetActor()->GetPhysicsSetup()->GetRagdollConfig(),
                jointDebugDrawDataFunction,
                Physics::CharacterPhysicsDebugDraw::ColorSettings{ renderActorSettings.m_ragdollColliderColor,
                                                                   renderActorSettings.m_selectedRagdollColliderColor,
                                                                   renderActorSettings.m_hoveredRagdollColliderColor,
                                                                   renderActorSettings.m_violatedJointLimitColor });
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
            EMotionFX::ActorInstance::ExpandBounds(aabb, instance->GetExpandBoundsBy());
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
            EMotionFX::ActorInstance::ExpandBounds(aabb, instance->GetExpandBoundsBy());
            if (aabb.IsValid())
            {
                auxGeom->DrawAabb(aabb, meshAabbColor, RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }

        if (enableStaticAabb)
        {
            auto& aabb = instance->GetAabb();
            if (aabb.IsValid())
            {
                auxGeom->DrawAabb(aabb, staticAabbColor, RPI::AuxGeomDraw::DrawStyle::Line);
            }
        }
    }

    AZ::Color AtomActorDebugDraw::GetModifiedColor(
        const AZ::Color& color,
        size_t jointIndex,
        const AZStd::unordered_set<size_t>* cachedSelectedJointIndices,
        size_t cachedHoveredJointIndex) const
    {
        if (cachedSelectedJointIndices && cachedSelectedJointIndices->find(jointIndex) != cachedSelectedJointIndices->end())
        {
            return SelectedColor;
        }
        else if (cachedHoveredJointIndex == jointIndex)
        {
            return HoveredColor;
        }
        else
        {
            return color;
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

        size_t cachedHoveredJointIndex = InvalidIndex;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedHoveredJointIndex, &EMotionFX::JointSelectionRequests::FindHoveredJointIndex, instance);

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

            renderColor = GetModifiedColor(color, parentIndex, cachedSelectedJointIndices, cachedHoveredJointIndex);

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

        size_t cachedHoveredJointIndex = InvalidIndex;
        EMotionFX::JointSelectionRequestBus::BroadcastResult(
            cachedHoveredJointIndex, &EMotionFX::JointSelectionRequests::FindHoveredJointIndex, instance);

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
            const float maxBoneScale = 0.05f;
            const float boneLength = bone.GetLengthEstimate();
            const float boneScale = AZStd::min(CalculateBoneScale(instance, joint), maxBoneScale);
            const float parentBoneScale = AZStd::min(CalculateBoneScale(instance, skeleton->GetNode(parentIndex)), maxBoneScale);
            const float cylinderSize = boneLength - boneScale - parentBoneScale;

            renderColor = GetModifiedColor(color, parentIndex, cachedSelectedJointIndices, cachedHoveredJointIndex);
            renderColor.SetA(0.5f);
            debugDisplay->SetColor(renderColor);

            // Render the bone cylinder, the cylinder will be directed towards the node's parent and must fit between the spheres
            debugDisplay->DrawSolidCylinder(centerWorldPos, boneDirection, boneScale, cylinderSize);
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

        const int oldState = debugDisplay->GetState();
        debugDisplay->DepthTestOff();

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

        debugDisplay->SetState(oldState);
    }

    void AtomActorDebugDraw::UpdateActorInstance(EMotionFX::ActorInstance* actorInstance, float deltaTime)
    {
        // Find the corresponding trajectory trace path for the given actor instance
        auto trajectoryPath = FindTrajectoryPath(actorInstance);
        if (!trajectoryPath)
        {
            return;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
        const uint32 particleSampleRate = 30;
        const float minLengthEstimate = 0.0001f;
        const float maxdeltaRot = 0.99;
        const uint32 maxNumberParticles = 50;
        if (motionExtractionNode)
        {
            const EMotionFX::Transform& worldTM = actorInstance->GetWorldSpaceTransform();

            // Add a particle to the trajectory path once we travel certain distance.
            bool distanceTravelledEnough = false;
            if (trajectoryPath->m_traceParticles.empty())
            {
                distanceTravelledEnough = true;
            }
            else
            {
                const size_t numParticles = trajectoryPath->m_traceParticles.size();
                const EMotionFX::Transform& oldWorldTM = trajectoryPath->m_traceParticles[numParticles - 1].m_worldTm;

                const AZ::Vector3& oldPos = oldWorldTM.m_position;
                const AZ::Quaternion oldRot = oldWorldTM.m_rotation.GetNormalized();
                const AZ::Quaternion rotation = worldTM.m_rotation.GetNormalized();

                const AZ::Vector3 deltaPos = worldTM.m_position - oldPos;
                const float deltaRot = AZStd::abs(rotation.Dot(oldRot));
                if (deltaPos.GetLengthEstimate() > minLengthEstimate || deltaRot < maxdeltaRot)
                {
                    distanceTravelledEnough = true;
                }
            }

            // Add the time delta to the time passed since the last add
            trajectoryPath->m_timePassed += deltaTime;
            if (trajectoryPath->m_timePassed >= (1.0f / particleSampleRate) && distanceTravelledEnough)
            {
                // Create the particle, fill its data and add it to the trajectory trace path
                TrajectoryPathParticle trajectoryParticle;
                trajectoryParticle.m_worldTm = worldTM;
                trajectoryPath->m_traceParticles.emplace_back(trajectoryParticle);

                // Reset the time passed as we just added a new particle
                trajectoryPath->m_timePassed = 0.0f;
            }
        }

        // Make sure we don't have too many items in our array
        if (trajectoryPath->m_traceParticles.size() > maxNumberParticles)
        {
            trajectoryPath->m_traceParticles.erase(begin(trajectoryPath->m_traceParticles));
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

    // Find the trajectory path for a given actor instance
    AtomActorDebugDraw::TrajectoryTracePath* AtomActorDebugDraw::FindTrajectoryPath(const EMotionFX::ActorInstance* actorInstance)
    {
        for (const auto& trajectoryPath : m_trajectoryTracePaths)
        {
            if (trajectoryPath->m_actorInstance == actorInstance)
            {
                return trajectoryPath.get();
            }
        }

        // We haven't created a path for the given actor instance yet, do so
        auto tracePath = AZStd::make_unique<TrajectoryTracePath>();

        tracePath->m_actorInstance = actorInstance;
        tracePath->m_traceParticles.reserve(512);

        return m_trajectoryTracePaths.emplace_back(AZStd::move(tracePath)).get();
    }

    void AtomActorDebugDraw::RenderTrajectoryPath(AzFramework::DebugDisplayRequests* debugDisplay,
        const EMotionFX::ActorInstance* actorInstance,
        const AZ::Color& headColor,
        const AZ::Color& pathColor)
    {
        auto trajectoryPath = FindTrajectoryPath(actorInstance);
        if (!trajectoryPath)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        EMotionFX::Node* extractionNode = actor->GetMotionExtractionNode();
        if (!extractionNode)
        {
            return;
        }

        // Fast access to the trajectory trace particles
        const AZStd::vector<TrajectoryPathParticle>& traceParticles = trajectoryPath->m_traceParticles;
        if (traceParticles.empty())
        {
            return;
        }

        const size_t numTraceParticles = traceParticles.size();
        const float trailWidthHalf = 0.25f;
        const float trailLength = 2.0f;
        const float arrowWidthHalf = 0.75f;
        const float arrowLength = 1.5f;
        const AZ::Vector3 liftFromGround(0.0f, 0.0f, 0.0001f);

        const AZ::Transform trajectoryWorldTM = actorInstance->GetWorldSpaceTransform().ToAZTransform();

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // Render arrow head
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // Get the position and some direction vectors of the trajectory node matrix
        EMotionFX::Transform worldTM = traceParticles[numTraceParticles - 1].m_worldTm;
        const AZ::Vector3 right = trajectoryWorldTM.GetBasisX().GetNormalized();
        const AZ::Vector3 center = trajectoryWorldTM.GetTranslation();
        const AZ::Vector3 forward = trajectoryWorldTM.GetBasisY().GetNormalized();
        const AZ::Vector3 up(0.0f, 0.0f, 1.0f);

        AZ::Vector3 vertices[7];
        AZ::Vector3 oldLeft, oldRight;

        /*
                            4
                           / \
                          /   \
                        /       \
                      /           \
                    /               \
                  5-----6       2-----3
                        |       |
                        |       |
                        |       |
                        |       |
                        |       |
                        0-------1
        */
        // Construct the arrow vertices
        float scale = 0.2f;
        vertices[0] = center + (-right * trailWidthHalf - forward * trailLength) * scale;
        vertices[1] = center + (right * trailWidthHalf - forward * trailLength) * scale;
        vertices[2] = center + (right * trailWidthHalf) * scale;
        vertices[3] = center + (right * arrowWidthHalf) * scale;
        vertices[4] = center + (forward * arrowLength) * scale;
        vertices[5] = center + (-right * arrowWidthHalf) * scale;
        vertices[6] = center + (-right * trailWidthHalf) * scale;

        oldLeft = vertices[6];
        oldRight = vertices[2];

        AZ::Vector3 arrowOldLeft = oldLeft;
        AZ::Vector3 arrowOldRight = oldRight;

        // Render the solid arrow
        debugDisplay->SetColor(headColor);
        debugDisplay->DrawTri(vertices[3] + liftFromGround, vertices[4] + liftFromGround, vertices[2] + liftFromGround);
        debugDisplay->DrawTri(vertices[2] + liftFromGround, vertices[4] + liftFromGround, vertices[6] + liftFromGround);
        debugDisplay->DrawTri(vertices[6] + liftFromGround, vertices[4] + liftFromGround, vertices[5] + liftFromGround);

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // Render arrow tail (actual path)
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        AZ::Vector3 a, b;
        AZ::Color color = pathColor;

        // Render the path from the arrow head towards the tail
        for (size_t i = numTraceParticles - 1; i > 0; i--)
        {
            // Calculate the normalized distance to the head, this value also represents the alpha value as it fades away while getting
            // closer to the end
            float normalizedDistance = (float)i / numTraceParticles;

            // Get the start and end point of the line segment and calculate the delta between them
            worldTM = traceParticles[i].m_worldTm;
            a = worldTM.m_position;
            b = traceParticles[i - 1].m_worldTm.m_position;
            AZ::Vector3 particleRight = worldTM.ToAZTransform().GetBasisX().GetNormalized();

            if (i > 1 && i < numTraceParticles - 3)
            {
                const AZ::Vector3 deltaA = traceParticles[i - 2].m_worldTm.m_position - traceParticles[i - 1].m_worldTm.m_position;
                const AZ::Vector3 deltaB = traceParticles[i - 1].m_worldTm.m_position - traceParticles[i].m_worldTm.m_position;
                const AZ::Vector3 deltaC = traceParticles[i].m_worldTm.m_position - traceParticles[i + 1].m_worldTm.m_position;
                const AZ::Vector3 deltaD = traceParticles[i + 1].m_worldTm.m_position - traceParticles[i + 2].m_worldTm.m_position;
                AZ::Vector3 delta = deltaA + deltaB + deltaC + deltaD;
                delta = delta.GetNormalizedSafe();

                particleRight = up.Cross(delta);
            }

            /*
                    //              .
                    //              .
                    //              .
                    //(oldLeft) 0   a   1 (oldRight)
                    //          |       |
                    //          |       |
                    //          |       |
                    //          |       |
                    //          |       |
                    //          2---b---3
            */

            // Construct the arrow vertices
            vertices[0] = oldLeft;
            vertices[1] = oldRight;
            vertices[2] = b + AZ::Vector3(-particleRight * trailWidthHalf) * scale;
            vertices[3] = b + AZ::Vector3(particleRight * trailWidthHalf) * scale;

            // Make sure we perfectly align with the arrow head
            if (i == numTraceParticles - 1)
            {
                normalizedDistance = 1.0f;
                vertices[0] = arrowOldLeft;
                vertices[1] = arrowOldRight;
            }

            // Render the solid arrow
            color.SetA(normalizedDistance);
            debugDisplay->SetColor(color);
            debugDisplay->DrawTri(vertices[0] + liftFromGround, vertices[2] + liftFromGround, vertices[1] + liftFromGround);
            debugDisplay->DrawTri(vertices[1] + liftFromGround, vertices[2] + liftFromGround, vertices[3] + liftFromGround);

            // Overwrite the old left and right values so that they can be used for the next trace particle
            oldLeft = vertices[2];
            oldRight = vertices[3];
        }
    }

    void AtomActorDebugDraw::RenderRootMotion(AzFramework::DebugDisplayRequests* debugDisplay,
        const EMotionFX::ActorInstance* actorInstance,
        const AZ::Color& rootColor)
    {
        const AZ::Transform actorTransform = actorInstance->GetWorldSpaceTransform().ToAZTransform();

        // Render two circle around the character position.
        debugDisplay->SetColor(rootColor);
        debugDisplay->DrawCircle(actorTransform.GetTranslation(), 1.0f);
        debugDisplay->DrawCircle(actorTransform.GetTranslation(), 0.05f);

        // Render the character facing direction.
        const AZ::Vector3 forward = actorTransform.GetBasisY();
        debugDisplay->DrawArrow(actorTransform.GetTranslation(), actorTransform.GetTranslation() + forward);
    }
} // namespace AZ::Render
