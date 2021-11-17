/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/MaterialBus.h>
#include <cfloat>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/MathConversion.h>
#include <Source/RigidBody.h>
#include <Source/Scene/PhysXScene.h>
#include <Source/Shape.h>

namespace PhysX
{
    namespace Utils
    {
        namespace Characters
        {
            AZ::Outcome<size_t> GetNodeIndex(const Physics::RagdollConfiguration& configuration, const AZStd::string& nodeName)
            {
                const size_t numNodes = configuration.m_nodes.size();
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    if (configuration.m_nodes[nodeIndex].m_debugName == nodeName)
                    {
                        return AZ::Success(nodeIndex);
                    }
                }

                return AZ::Failure();
            }

            /// Adds the properties that exist in both the PhysX capsule and box controllers to the controller description.
            /// @param[in,out] controllerDesc The controller description to which the shape independent properties should be added.
            /// @param characterConfig Information about the character required for initialization.
            static void AppendShapeIndependentProperties(physx::PxControllerDesc& controllerDesc,
                const Physics::CharacterConfiguration& characterConfig, CharacterControllerCallbackManager* callbackManager)
            {
                AZStd::vector<AZStd::shared_ptr<Physics::Material>> materials;

                if (characterConfig.m_materialSelection.GetMaterialIdsAssignedToSlots().empty())
                {
                    // If material selection has no slots, falling back to default material.
                    AZStd::shared_ptr<Physics::Material> defaultMaterial;
                    Physics::PhysicsMaterialRequestBus::BroadcastResult(defaultMaterial,
                        &Physics::PhysicsMaterialRequestBus::Events::GetGenericDefaultMaterial);
                    if (!defaultMaterial)
                    {
                        AZ_Error("PhysX Character Controller", false, "Invalid default material.");
                        return;
                    }
                    materials.push_back(AZStd::move(defaultMaterial));
                }
                else
                {
                    Physics::PhysicsMaterialRequestBus::Broadcast(
                        &Physics::PhysicsMaterialRequestBus::Events::GetMaterials,
                        characterConfig.m_materialSelection,
                        materials);
                    if (materials.empty())
                    {
                        AZ_Error("PhysX Character Controller", false, "Could not create character controller, material list was empty.");
                        return;
                    }
                }

                physx::PxMaterial* pxMaterial = static_cast<physx::PxMaterial*>(materials.front()->GetNativePointer());

                controllerDesc.material = pxMaterial;
                controllerDesc.slopeLimit = cosf(AZ::DegToRad(characterConfig.m_maximumSlopeAngle));
                controllerDesc.stepOffset = characterConfig.m_stepHeight;
                controllerDesc.upDirection = characterConfig.m_upDirection.IsZero()
                    ? physx::PxVec3(0.0f, 0.0f, 1.0f)
                    : PxMathConvert(characterConfig.m_upDirection).getNormalized();
                controllerDesc.userData = nullptr;
                controllerDesc.behaviorCallback = callbackManager;
                controllerDesc.reportCallback = callbackManager;
            }

            /// Adds the properties which are PhysX specific and not included in the base generic character configuration.
            /// @param[in,out] controllerDesc The controller description to which the PhysX specific properties should be added.
            /// @param characterConfig Information about the character required for initialization.
            void AppendPhysXSpecificProperties(physx::PxControllerDesc& controllerDesc,
                const Physics::CharacterConfiguration& characterConfig)
            {
                if (characterConfig.RTTI_GetType() == CharacterControllerConfiguration::RTTI_Type())
                {
                    const auto& extendedConfig = static_cast<const CharacterControllerConfiguration&>(characterConfig);

                    controllerDesc.scaleCoeff = extendedConfig.m_scaleCoefficient;
                    controllerDesc.contactOffset = extendedConfig.m_contactOffset;
                    controllerDesc.nonWalkableMode = extendedConfig.m_slopeBehaviour == SlopeBehaviour::PreventClimbing
                        ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING
                        : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
                }
            }

            CharacterController* CreateCharacterController(PhysXScene* scene,
                const Physics::CharacterConfiguration& characterConfig)
            {
                if (scene == nullptr)
                {
                    AZ_Error("PhysX Character Controller", false, "Failed to create character controller as the scene is null");
                    return nullptr;
                }

                physx::PxControllerManager* manager = scene->GetOrCreateControllerManager();
                if (manager == nullptr)
                {
                    AZ_Error("PhysX Character Controller", false, "Could not retrieve character controller manager.");
                    return nullptr;
                }

                auto callbackManager = AZStd::make_unique<CharacterControllerCallbackManager>();

                physx::PxController* pxController = nullptr;
                auto* pxScene = static_cast<physx::PxScene*>(scene->GetNativePointer());

                switch (characterConfig.m_shapeConfig->GetShapeType())
                {
                case Physics::ShapeType::Capsule:
                    {
                        physx::PxCapsuleControllerDesc capsuleDesc;

                        const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(*characterConfig.m_shapeConfig);
                        // LY height means total height, PhysX means height of straight section
                        capsuleDesc.height = AZ::GetMax(epsilon, capsuleConfig.m_height - 2.0f * capsuleConfig.m_radius);
                        capsuleDesc.radius = capsuleConfig.m_radius;
                        capsuleDesc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;

                        AppendShapeIndependentProperties(capsuleDesc, characterConfig, callbackManager.get());
                        AppendPhysXSpecificProperties(capsuleDesc, characterConfig);
                        PHYSX_SCENE_WRITE_LOCK(pxScene);
                        pxController = manager->createController(capsuleDesc); // This internally adds the controller's actor to the scene
                    }
                    break;
                case Physics::ShapeType::Box:
                    {
                        physx::PxBoxControllerDesc boxDesc;

                        const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(*characterConfig.m_shapeConfig);
                        boxDesc.halfHeight = 0.5f * boxConfig.m_dimensions.GetZ();
                        boxDesc.halfSideExtent = 0.5f * boxConfig.m_dimensions.GetY();
                        boxDesc.halfForwardExtent = 0.5f * boxConfig.m_dimensions.GetX();

                        AppendShapeIndependentProperties(boxDesc, characterConfig, callbackManager.get());
                        AppendPhysXSpecificProperties(boxDesc, characterConfig);
                        PHYSX_SCENE_WRITE_LOCK(pxScene);
                        pxController = manager->createController(boxDesc); // This internally adds the controller's actor to the scene
                    }
                    break;
                default:
                    {
                        AZ_Error("PhysX Character Controller", false, "PhysX only supports box and capsule shapes for character controllers.");
                        return nullptr;
                    }
                    break;
                }

                if (!pxController)
                {
                    AZ_Error("PhysX Character Controller", false, "Failed to create character controller.");
                    return nullptr;
                }

                return aznew CharacterController(pxController, AZStd::move(callbackManager), scene->GetSceneHandle());
            }

            Ragdoll* CreateRagdoll(Physics::RagdollConfiguration& configuration, AzPhysics::SceneHandle sceneHandle)
            {
                const size_t numNodes = configuration.m_nodes.size();
                if (numNodes != configuration.m_initialState.size())
                {
                    AZ_Error("PhysX Ragdoll", false, "Mismatch between number of nodes in ragdoll configuration (%i) "
                        "and number of nodes in the initial ragdoll state (%i)", numNodes, configuration.m_initialState.size());
                    return nullptr;
                }

                AZStd::unique_ptr<Ragdoll> ragdoll = AZStd::make_unique<Ragdoll>(sceneHandle);
                ragdoll->SetParentIndices(configuration.m_parentIndices);

                auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
                if (sceneInterface == nullptr)
                {
                    AZ_Error("PhysX Ragdoll", false, "Unable to Create Ragdoll, Physics Scene Interface is missing.");
                    return nullptr;
                }

                // Set up rigid bodies
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    Physics::RagdollNodeConfiguration& nodeConfig = configuration.m_nodes[nodeIndex];
                    const Physics::RagdollNodeState& nodeState = configuration.m_initialState[nodeIndex];

                    Physics::CharacterColliderNodeConfiguration* colliderNodeConfig = configuration.m_colliders.FindNodeConfigByName(nodeConfig.m_debugName);
                    if (colliderNodeConfig)
                    {
                        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
                        for (const auto& [colliderConfig, shapeConfig] : colliderNodeConfig->m_shapes)
                        {
                            if (colliderConfig == nullptr || shapeConfig == nullptr)
                            {
                                AZ_Error("PhysX Ragdoll", false, "Failed to create collider shape for ragdoll node %s", nodeConfig.m_debugName.c_str());
                                return nullptr;
                            }

                            if (auto shape = AZStd::make_shared<Shape>(*colliderConfig, *shapeConfig))
                            {
                                shapes.emplace_back(shape);
                            }
                            else
                            {
                                AZ_Error("PhysX Ragdoll", false, "Failed to create collider shape for ragdoll node %s", nodeConfig.m_debugName.c_str());
                                return nullptr;
                            }
                        }
                        nodeConfig.m_colliderAndShapeData = shapes;
                    }
                    nodeConfig.m_startSimulationEnabled = false;
                    nodeConfig.m_position = nodeState.m_position;
                    nodeConfig.m_orientation = nodeState.m_orientation;

                    AZStd::unique_ptr<RagdollNode> node = AZStd::make_unique<RagdollNode>(sceneHandle, nodeConfig);
                    if (node->GetRigidBodyHandle() != AzPhysics::InvalidSimulatedBodyHandle)
                    {
                        ragdoll->AddNode(AZStd::move(node));
                    }
                    else
                    {
                        AZ_Error("PhysX Ragdoll", false, "Failed to create rigid body for ragdoll node %s", nodeConfig.m_debugName.c_str());
                        node.reset();
                    }
                }

                // Set up joints.  Needs a second pass because child nodes in the ragdoll config aren't guaranteed to have
                // larger indices than their parents.
                size_t rootIndex = SIZE_MAX;
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    size_t parentIndex = configuration.m_parentIndices[nodeIndex];
                    if (parentIndex < numNodes)
                    {
                        physx::PxRigidDynamic* parentActor = ragdoll->GetPxRigidDynamic(parentIndex);
                        physx::PxRigidDynamic* childActor = ragdoll->GetPxRigidDynamic(nodeIndex);
                        if (parentActor && childActor)
                        {
                            physx::PxVec3 parentOffset = parentActor->getGlobalPose().q.rotateInv(
                                childActor->getGlobalPose().p - parentActor->getGlobalPose().p);
                            physx::PxTransform parentTM(parentOffset);
                            physx::PxTransform childTM(physx::PxIdentity);

                            AZStd::shared_ptr<AzPhysics::JointConfiguration> jointConfig = configuration.m_nodes[nodeIndex].m_jointConfig;
                            if (!jointConfig)
                            {
                                jointConfig = AZStd::make_shared<D6JointLimitConfiguration>();
                            }
                            
                            AzPhysics::JointHandle jointHandle = sceneInterface->AddJoint(
                                sceneHandle, jointConfig.get(), 
                                ragdoll->GetNode(parentIndex)->GetRigidBody().m_bodyHandle,
                                ragdoll->GetNode(nodeIndex)->GetRigidBody().m_bodyHandle);

                            AzPhysics::Joint* joint = sceneInterface->GetJointFromHandle(sceneHandle, jointHandle);

                            if (!joint)
                            {
                                AZ_Error("PhysX Ragdoll", false, "Failed to create joint for node index %i.", nodeIndex);
                                return nullptr;
                            }

                            // Moving from PhysX 3.4 to 4.1, the allowed range of the twist angle was expanded from -pi..pi
                            // to -2*pi..2*pi.
                            // In 3.4, twist angles which were outside the range were wrapped into it, which means that it
                            // would be possible for a joint to have been authored under 3.4 which would be inside its twist
                            // limit in 3.4 but violating the limit by up to 2*pi in 4.1.
                            // If this case is detected, flipping the sign of one of the joint local pose quaternions will
                            // ensure that the twist angle will have a value which would not lead to wrapping.
                            auto* jointNativePointer = static_cast<physx::PxJoint*>(joint->GetNativePointer());
                            if (jointNativePointer && jointNativePointer->getConcreteType() == physx::PxJointConcreteType::eD6)
                            {
                                auto* d6Joint = static_cast<physx::PxD6Joint*>(jointNativePointer);
                                const float twist = d6Joint->getTwistAngle();
                                const physx::PxJointAngularLimitPair twistLimit = d6Joint->getTwistLimit();
                                if (twist < twistLimit.lower || twist > twistLimit.upper)
                                {
                                    physx::PxTransform childLocalTransform = d6Joint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
                                    childLocalTransform.q = -childLocalTransform.q;
                                    d6Joint->setLocalPose(physx::PxJointActorIndex::eACTOR1, childLocalTransform);
                                }
                            }

                            Physics::RagdollNode* childNode = ragdoll->GetNode(nodeIndex);
                            static_cast<RagdollNode*>(childNode)->SetJoint(joint);
                        }
                        else
                        {
                            AZ_Error("PhysX Ragdoll", false, "Failed to create joint for node index %i.", nodeIndex);
                            return nullptr;
                        }
                    }
                    else
                    {
                        // If the configuration only has one root and is valid, the node without a parent must be the root.
                        rootIndex = nodeIndex;
                    }
                }

                ragdoll->SetRootIndex(rootIndex);
                
                return ragdoll.release();
            }

            physx::PxD6JointDrive CreateD6JointDrive(float stiffness, float dampingRatio, float forceLimit)
            {
                if (!(std::isfinite)(stiffness) || stiffness < 0.0f)
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint stiffness, using 0.0f instead.");
                    stiffness = 0.0f;
                }

                if (!(std::isfinite)(dampingRatio) || dampingRatio < 0.0f)
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint damping ratio, using 1.0f instead.");
                    dampingRatio = 1.0f;
                }

                if (!(std::isfinite)(forceLimit))
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint force limit, ignoring.");
                    forceLimit = std::numeric_limits<float>::max();
                }

                float damping = dampingRatio * 2.0f * sqrtf(stiffness);
                bool isAcceleration = true;
                return physx::PxD6JointDrive(stiffness, damping, forceLimit, isAcceleration);
            }

            AZStd::vector<DepthData> ComputeHierarchyDepths(const AZStd::vector<size_t>& parentIndices)
            {
                const size_t numNodes = parentIndices.size();
                AZStd::vector<DepthData> nodeDepths(numNodes);
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    nodeDepths[nodeIndex] = { -1, nodeIndex };
                }

                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    if (nodeDepths[nodeIndex].m_depth != -1)
                    {
                        continue;
                    }
                    int depth = -1; // initial depth value for this node
                    int ancestorDepth = 0; // the depth of the first ancestor we find when iteratively visiting parents
                    bool ancestorFound = false; // whether we have found either an ancestor which already has a depth value, or the root
                    size_t currentIndex = nodeIndex;
                    while (!ancestorFound)
                    {
                        depth++;
                        if (depth > numNodes)
                        {
                            AZ_Error("PhysX Ragdoll", false, "Loop detected in hierarchy depth computation.");
                            return nodeDepths;
                        }
                        const size_t parentIndex = parentIndices[currentIndex];

                        if (parentIndex >= numNodes || nodeDepths[currentIndex].m_depth != -1)
                        {
                            ancestorFound = true;
                            ancestorDepth = (nodeDepths[currentIndex].m_depth != -1) ? nodeDepths[currentIndex].m_depth : 0;
                        }

                        currentIndex = parentIndex;
                    }

                    currentIndex = nodeIndex;
                    for (int i = depth; i >= 0; i--)
                    {
                        nodeDepths[currentIndex] = { ancestorDepth + i, currentIndex };
                        currentIndex = parentIndices[currentIndex];
                    }
                }

                return nodeDepths;
            }
        } // namespace Characters
    } // namespace Utils
} // namespace PhysX
