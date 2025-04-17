/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Capsule.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Math/Sphere.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>
#include <AzFramework/Physics/DebugDraw/CharacterPhysicsDebugDraw.h>

namespace Physics
{
    void CharacterPhysicsDebugDraw::JointLimitRenderBuffers::Clear()
    {
        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_lineBuffer.clear();
        m_lineValidityBuffer.clear();
    }

    void CharacterPhysicsDebugDraw::RenderColliders(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::CharacterColliderNodeConfiguration& nodeConfig,
        NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
        const ColorSettings& colorSettings,
        uint32_t invalidColliderBitArray)
    {
        NodeDebugDrawData nodeDebugDrawData = nodeDebugDrawDataFunction(nodeConfig);
        if (!nodeDebugDrawData.m_valid)
        {
            return;
        }

        debugDisplay->DepthTestOff();

        const AzPhysics::ShapeColliderPairList& colliders = nodeConfig.m_shapes;
        for (size_t colliderIndex = 0; colliderIndex < colliders.size(); colliderIndex++)
        {
            const auto& collider = colliders[colliderIndex];
            AZ::Color colliderColor;
            if (nodeDebugDrawData.m_selected)
            {
                colliderColor = colorSettings.m_selectedColor;
            }
            else if (nodeDebugDrawData.m_hovered)
            {
                colliderColor = colorSettings.m_hoveredColor;
            }
            else if ((1 << colliderIndex) & invalidColliderBitArray)
            {
                colliderColor = colorSettings.m_errorColor;
            }
            else
            {
                colliderColor = colorSettings.m_defaultColor;
            }
            debugDisplay->SetColor(colliderColor);

            const AZ::Transform colliderOffsetTransform =
                AZ::Transform::CreateFromQuaternionAndTranslation(collider.first->m_rotation, collider.first->m_position);
            const AZ::Transform colliderGlobalTransform = nodeDebugDrawData.m_worldTransform * colliderOffsetTransform;

            const AZ::TypeId colliderType = collider.second->RTTI_GetType();
            if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
            {
                auto* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireSphere(colliderGlobalTransform.GetTranslation(), sphere->m_radius);
            }
            else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
            {
                auto* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireCapsule(
                    colliderGlobalTransform.GetTranslation(), colliderGlobalTransform.GetBasisZ(), capsule->m_radius,
                    AZ::GetMax(0.0f, capsule->m_height - 2.0f * capsule->m_radius));
            }
            else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
            {
                auto* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());
                debugDisplay->DrawWireOBB(
                    colliderGlobalTransform.GetTranslation(), colliderGlobalTransform.GetBasisX(),
                    colliderGlobalTransform.GetBasisY(), colliderGlobalTransform.GetBasisZ(), 0.5f * box->m_dimensions);
            }
        }
    }

    AZStd::vector<uint32_t> ComputeInvalidColliderBitArrays(
        const Physics::CharacterColliderConfiguration* colliderConfig,
        const Physics::ParentIndices& parentIndices,
        CharacterPhysicsDebugDraw::NodeDebugDrawDataFunction nodeDebugDrawDataFunction)
    {
        const size_t numNodes = colliderConfig->m_nodes.size();
        AZStd::vector<uint32_t> invalidColliderBitArrays(numNodes, 0u);
        if (numNodes < 2)
        {
            return invalidColliderBitArrays;
        }
        for (size_t nodeIndex1 = 0; nodeIndex1 < numNodes - 1; nodeIndex1++)
        {
            AZ::Transform node1Transform = nodeDebugDrawDataFunction(colliderConfig->m_nodes[nodeIndex1]).m_worldTransform;
            const size_t numShapes1 = colliderConfig->m_nodes[nodeIndex1].m_shapes.size();
            for (size_t nodeIndex2 = nodeIndex1 + 1; nodeIndex2 < numNodes; nodeIndex2++)
            {
                // check if either of the nodes is the parent of the other
                if ((nodeIndex1 < parentIndices.size() && parentIndices[nodeIndex1] == nodeIndex2) ||
                    (nodeIndex2 < parentIndices.size() && parentIndices[nodeIndex2] == nodeIndex1))
                {
                    continue;
                }

                AZ::Transform node2Transform = nodeDebugDrawDataFunction(colliderConfig->m_nodes[nodeIndex2]).m_worldTransform;

                const size_t numShapes2 = colliderConfig->m_nodes[nodeIndex2].m_shapes.size();
                for (size_t shapeIndex1 = 0; shapeIndex1 < numShapes1; shapeIndex1++)
                {
                    const AzPhysics::ShapeColliderPair& shapeColliderPair1 = colliderConfig->m_nodes[nodeIndex1].m_shapes[shapeIndex1];
                    const AZ::Transform shape1Transform = node1Transform * AZ::Transform::CreateFromQuaternionAndTranslation(
                        shapeColliderPair1.first->m_rotation, shapeColliderPair1.first->m_position);

                    for (size_t shapeIndex2 = 0; shapeIndex2 < numShapes2; shapeIndex2++)
                    {
                        const AzPhysics::ShapeColliderPair& shapeColliderPair2 = colliderConfig->m_nodes[nodeIndex2].m_shapes[shapeIndex2];
                        const AZ::Transform shape2Transform = node2Transform * AZ::Transform::CreateFromQuaternionAndTranslation(
                            shapeColliderPair2.first->m_rotation, shapeColliderPair2.first->m_position);

                        // check the collision layers
                        bool shouldCollide = false;
                        Physics::CollisionRequestBus::BroadcastResult(
                            shouldCollide, &Physics::CollisionRequests::ShouldCollide, *shapeColliderPair1.first.get(),
                            *shapeColliderPair2.first.get());

                        if (!shouldCollide)
                        {
                            continue;
                        }

                        // test if the geometries of the colliders intersect
                        using ShapeVariant = AZStd::variant<AZStd::monostate, AZ::Capsule, AZ::Obb, AZ::Sphere>;

                        auto getShapeVariant = [](const Physics::ShapeConfiguration* shapeConfiguration,
                                                  const AZ::Transform& shapeTransform) -> ShapeVariant
                        {
                            if (const auto* boxShapeConfiguration =
                                    azdynamic_cast<const Physics::BoxShapeConfiguration*>(shapeConfiguration))
                            {
                                return boxShapeConfiguration->ToObb(shapeTransform);
                            }
                            else if (
                                const auto* capsuleShapeConfiguration =
                                    azdynamic_cast<const Physics::CapsuleShapeConfiguration*>(shapeConfiguration))
                            {
                                return capsuleShapeConfiguration->ToCapsule(shapeTransform);
                            }
                            else if (
                                const auto* sphereShapeConfiguration =
                                    azdynamic_cast<const Physics::SphereShapeConfiguration*>(shapeConfiguration))
                            {
                                return sphereShapeConfiguration->ToSphere(shapeTransform);
                            }
                            else
                            {
                                return AZStd::monostate{};
                            }
                        };

                        ShapeVariant shapeVariant1 = getShapeVariant(shapeColliderPair1.second.get(), shape1Transform);
                        ShapeVariant shapeVariant2 = getShapeVariant(shapeColliderPair2.second.get(), shape2Transform);
                        bool overlaps = AZStd::visit(
                            [](auto&& arg1, auto&& arg2)
                            {
                                using Arg1Type = AZStd::decay_t<decltype(arg1)>;
                                using Arg2Type = AZStd::decay_t<decltype(arg2)>;
                                if constexpr (AZStd::is_same_v<Arg1Type, AZStd::monostate> || AZStd::is_same_v<Arg2Type, AZStd::monostate>)
                                {
                                    return false;
                                }
                                else
                                {
                                    return AZ::ShapeIntersection::Overlaps(arg1, arg2);
                                }
                            },
                            shapeVariant1, shapeVariant2);
                        if (overlaps)
                        {
                            invalidColliderBitArrays[nodeIndex1] |= (1 << shapeIndex1);
                            invalidColliderBitArrays[nodeIndex2] |= (1 << shapeIndex2);
                        }
                    }
                }
            }
        }
        return invalidColliderBitArrays;
    }

    void CharacterPhysicsDebugDraw::RenderColliders(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::CharacterColliderConfiguration* colliderConfig,
        NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
        const ColorSettings& colorSettings)
    {
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
        {
            RenderColliders(debugDisplay, nodeConfig, nodeDebugDrawDataFunction, colorSettings);
        }
    }

    void CharacterPhysicsDebugDraw::RenderRagdollColliders(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::CharacterColliderConfiguration* colliderConfig,
        NodeDebugDrawDataFunction nodeDebugDrawDataFunction,
        const ParentIndices& parentIndices,
        const ColorSettings& colorSettings)
    {
        const AZStd::vector<uint32_t> invalidColliderBitArrays =
            ComputeInvalidColliderBitArrays(colliderConfig, parentIndices, nodeDebugDrawDataFunction);

        for (size_t nodeIndex = 0; nodeIndex < colliderConfig->m_nodes.size(); nodeIndex++)
        {
            const Physics::CharacterColliderNodeConfiguration& nodeConfig = colliderConfig->m_nodes[nodeIndex];
            const uint32_t invalidColliderBitArray = nodeIndex < invalidColliderBitArrays.size() ? invalidColliderBitArrays[nodeIndex] : 0;
            RenderColliders(debugDisplay, nodeConfig, nodeDebugDrawDataFunction, colorSettings, invalidColliderBitArray);
        }
    }

    void CharacterPhysicsDebugDraw::RenderJointFrame(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const RagdollNodeConfiguration& ragdollNodeConfig,
        const JointDebugDrawData& jointDebugDrawData,
        const ColorSettings& colorSettings)
    {
        if (ragdollNodeConfig.m_jointConfig)
        {
            const AZ::Transform jointChildWorldSpaceTransform = jointDebugDrawData.m_childWorldTransform *
                AZ::Transform::CreateFromQuaternion(ragdollNodeConfig.m_jointConfig->m_childLocalRotation);
            const AZ::Vector3 xAxisDirection = jointChildWorldSpaceTransform.GetBasisX();

            debugDisplay->SetColor(colorSettings.m_selectedColor);
            debugDisplay->DrawArrow(
                jointChildWorldSpaceTransform.GetTranslation(), jointChildWorldSpaceTransform.GetTranslation() + xAxisDirection, 0.1f);
        }
    };

    void CharacterPhysicsDebugDraw::RenderJointLimit(
        AzFramework::DebugDisplayRequests* debugDisplay,
        const RagdollNodeConfiguration& ragdollNodeConfig,
        const JointDebugDrawData& jointDebugDrawData,
        const ColorSettings& colorSettings)
    {
        m_jointLimitRenderBuffers.Clear();

        if (!ragdollNodeConfig.m_jointConfig)
        {
            return;
        }

        if (auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
        {
            jointHelpers->GenerateJointLimitVisualizationData(
                *ragdollNodeConfig.m_jointConfig, jointDebugDrawData.m_parentModelSpaceOrientation,
                jointDebugDrawData.m_childModelSpaceOrientation, s_scale, s_angularSubdivisions, s_radialSubdivisions,
                m_jointLimitRenderBuffers.m_vertexBuffer, m_jointLimitRenderBuffers.m_indexBuffer,
                m_jointLimitRenderBuffers.m_lineBuffer, m_jointLimitRenderBuffers.m_lineValidityBuffer);
        }

        const size_t numLineBufferEntries = m_jointLimitRenderBuffers.m_lineBuffer.size();
        if (m_jointLimitRenderBuffers.m_lineValidityBuffer.size() * 2 != numLineBufferEntries)
        {
            AZ_ErrorOnce(
                "Ragdoll Debug Draw", false, "Unexpected buffer size in joint limit visualization for node %s",
                ragdollNodeConfig.m_debugName.c_str());
            return;
        }

        for (size_t i = 0; i < numLineBufferEntries; i += 2)
        {
            const AZ::Color& lineColor =
                m_jointLimitRenderBuffers.m_lineValidityBuffer[i / 2] ? colorSettings.m_selectedColor : colorSettings.m_errorColor;
            debugDisplay->DepthTestOff();
            debugDisplay->DrawLine(
                jointDebugDrawData.m_parentWorldTransform.TransformPoint(m_jointLimitRenderBuffers.m_lineBuffer[i]),
                jointDebugDrawData.m_parentWorldTransform.TransformPoint(m_jointLimitRenderBuffers.m_lineBuffer[i + 1]),
                lineColor.GetAsVector4(), lineColor.GetAsVector4());
        }
    }

    void CharacterPhysicsDebugDraw::RenderJointLimits(AzFramework::DebugDisplayRequests* debugDisplay,
        const Physics::RagdollConfiguration& ragdollConfig,
        JointDebugDrawDataFunction jointDebugDrawDataFunction,
        const ColorSettings& colorSettings)
    {
        for (const auto& ragdollNodeConfig : ragdollConfig.m_nodes)
        {
            JointDebugDrawData jointDebugDrawData = jointDebugDrawDataFunction(ragdollNodeConfig);
            if (!jointDebugDrawData.m_valid || !jointDebugDrawData.m_visible)
            {
                continue;
            }

            RenderJointLimit(debugDisplay, ragdollNodeConfig, jointDebugDrawData, colorSettings);
            RenderJointFrame(debugDisplay, ragdollNodeConfig, jointDebugDrawData, colorSettings);
        }
    }
} // namespace Physics
