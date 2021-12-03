/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/PhysicsSetup.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(PhysicsSetup, EMotionFX::ActorAllocator, 0)

    const char* PhysicsSetup::s_colliderConfigTypeVisualNames[5] =
    {
        "Hit Detection",
        "Ragdoll",
        "Cloth",
        "Simulated Object",
        "Unknown"
    };

    Physics::AnimationConfiguration& PhysicsSetup::GetConfig()
    {
        return m_config;
    }

    Physics::CharacterColliderConfiguration& PhysicsSetup::GetHitDetectionConfig()
    {
        return m_config.m_hitDetectionConfig;
    }

    Physics::RagdollConfiguration& PhysicsSetup::GetRagdollConfig()
    {
        return m_config.m_ragdollConfig;
    }

    const Physics::RagdollConfiguration& PhysicsSetup::GetRagdollConfig() const
    {
        return m_config.m_ragdollConfig;
    }

    const char* PhysicsSetup::GetStringForColliderConfigType(ColliderConfigType configType)
    {
        static const char* colliderConfigTypeNames[5] =
        {
            "HitDetection",
            "Ragdoll",
            "Cloth",
            "SimulatedObjectCollider",
            "Unknown"
        };

        if (configType < Unknown)
        {
            return colliderConfigTypeNames[configType];
        }

        return colliderConfigTypeNames[Unknown];
    }

    const char* PhysicsSetup::GetVisualNameForColliderConfigType(ColliderConfigType configType)
    {
        if (configType < Unknown)
        {
            return s_colliderConfigTypeVisualNames[configType];
        }

        return s_colliderConfigTypeVisualNames[Unknown];
    }

    PhysicsSetup::ColliderConfigType PhysicsSetup::GetColliderConfigTypeFromString(const AZStd::string& configTypeString)
    {
        if (configTypeString == "HitDetection")
        {
            return ColliderConfigType::HitDetection;
        }
        else if (configTypeString == "Ragdoll")
        {
            return ColliderConfigType::Ragdoll;
        }
        else if (configTypeString == "Cloth")
        {
            return ColliderConfigType::Cloth;
        }
        else if (configTypeString == "SimulatedObjectCollider")
        {
            return ColliderConfigType::SimulatedObjectCollider;
        }

        return ColliderConfigType::Unknown;
    }

    Physics::CharacterColliderConfiguration* PhysicsSetup::GetColliderConfigByType(ColliderConfigType configType)
    {
        switch (configType)
        {
        case ColliderConfigType::HitDetection:
        {
            return &m_config.m_hitDetectionConfig;
        }
        case ColliderConfigType::Ragdoll:
        {
            return &m_config.m_ragdollConfig.m_colliders;
        }
        case ColliderConfigType::Cloth:
        {
            return &m_config.m_clothConfig;
        }
        case ColliderConfigType::SimulatedObjectCollider:
        {
            return &m_config.m_simulatedObjectColliderConfig;
        }
        }

        return nullptr;
    }

    void PhysicsSetup::LogRagdollConfig(Actor* actor, const char* title)
    {
        AZ_Printf("EMotionFX", "------------------------------------------------------------------------\n");
        AZ_Printf("EMotionFX", title);

        const Skeleton* skeleton = actor->GetSkeleton();
        const AZStd::vector<Physics::RagdollNodeConfiguration>& ragdollNodes = m_config.m_ragdollConfig.m_nodes;
        const size_t numRagdollNodes = ragdollNodes.size();

        AZ_Printf("EMotionFX", " + Ragdoll: Nodes=%d\n", numRagdollNodes);
        for (size_t i = 0; i < numRagdollNodes; ++i)
        {
            const Node* node = skeleton->FindNodeByName(ragdollNodes[i].m_debugName.c_str());
            if (node)
            {
                AZ_Printf("EMotionFX", "    - Ragdoll Node #%d (%s): Index=%d, Name=%s\n", i, ragdollNodes[i].m_debugName.c_str(), node->GetNodeIndex(), node->GetName());
            }
            else
            {
                AZ_Printf("EMotionFX", "    - Ragdoll Node #%d (%s): Error, ragdoll node not found in animation skeleton!\n", i, ragdollNodes[i].m_debugName.c_str());
            }
        }

        AZ_Printf("EMotionFX", "------------------------------------------------------------------------\n");
    }

    const Node* PhysicsSetup::FindRagdollParentNode(const Node* node) const
    {
        if (!node)
        {
            AZ_Error("EMotionFX", false, "Invalid input node in FindRagdollParentNode");
            return nullptr;
        }

        const Node* currentNode = node->GetParentNode();
        while (currentNode)
        {
            if (m_config.m_ragdollConfig.FindNodeConfigByName(currentNode->GetNameString()))
            {
                return currentNode;
            }
            currentNode = currentNode->GetParentNode();
        }

        return nullptr;
    }

    Physics::CharacterColliderConfiguration& PhysicsSetup::GetClothConfig()
    {
        return m_config.m_clothConfig;
    }

    const Physics::CharacterColliderConfiguration& PhysicsSetup::GetClothConfig() const
    {
        return m_config.m_clothConfig;
    }

    Physics::CharacterColliderConfiguration& PhysicsSetup::GetSimulatedObjectColliderConfig()
    {
        return m_config.m_simulatedObjectColliderConfig;
    }

    const Physics::CharacterColliderConfiguration& PhysicsSetup::GetSimulatedObjectColliderConfig() const
    {
        return m_config.m_simulatedObjectColliderConfig;
    }

    AZ::Outcome<AzPhysics::ShapeColliderPair> PhysicsSetup::CreateColliderByType(const AZ::TypeId& typeId)
    {
        AZStd::string outResult;
        return CreateColliderByType(typeId, outResult);
    }

    AZ::Outcome<AzPhysics::ShapeColliderPair> PhysicsSetup::CreateColliderByType(const AZ::TypeId& typeId, AZStd::string& outResult)
    {
        if (typeId.IsNull())
        {
            outResult = "Cannot add collider. Type is is null.";
            return AZ::Failure();
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            outResult = "Can't get serialize context from component application.";
            return AZ::Failure();
        }

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(typeId);
        if (!classData)
        {
            outResult = "Cannot add collider. Class data cannot be found.";
            return AZ::Failure();
        }

        AZStd::shared_ptr<Physics::ShapeConfiguration> shapeConfig(
            reinterpret_cast<Physics::ShapeConfiguration*>(classData->m_factory->Create(classData->m_name)));

        if (!shapeConfig)
        {
            outResult = AZStd::string::format("Could not create collider with type '%s'.", typeId.ToString<AZStd::string>().c_str());
            return AZ::Failure();
        }
        
        AzPhysics::ShapeColliderPair pair(AZStd::make_shared<Physics::ColliderConfiguration>(), shapeConfig);
        if (pair.first->m_materialSelection.GetMaterialIdsAssignedToSlots().empty())
        {
            pair.first->m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        }
        return AZ::Success(pair);
    }

    void PhysicsSetup::AutoSizeCollider(AzPhysics::ShapeColliderPair& collider, const Actor* actor, const Node* joint)
    {
        if (!collider.second || !actor || !joint)
        {
            return;
        }

        const Pose* pose = actor->GetBindPose();
        const AZ::Transform nodeTransform = pose->GetModelSpaceTransform(joint->GetNodeIndex()).ToAZTransform();
        const AZ::Transform nodeTransformInverse = nodeTransform.GetInverse();

        const AZ::Vector3 boneDirection = GetBoneDirection(actor->GetSkeleton(), joint);
        // set a minimum bone length to avoid any problems in the edge case where the bone direction is zero length
        const float minBoneLength = 1e-2f;
        const float boneLength = AZ::GetMax(minBoneLength, boneDirection.GetLength());

        // constant to ensure capsule radius is slightly less than half the height to avoid malformed capsules
        const float minRadiusRatio = 0.499f;

        // get the mesh points for which this bone is the biggest influence
        AZStd::vector<AZ::Vector3> meshPoints;
        actor->FindMostInfluencedMeshPoints(joint, meshPoints);

        // estimate of the radial extent of the mesh geometry connected to this bone
        // initialize it to just under half the bone length to handle the edge case where no mesh points are found
        float rootMeanSquareDistanceFromBone = minRadiusRatio * boneLength;

        // translate each mesh point relative to the center of the bone, and remove the component parallel to the bone axis
        const size_t numMeshPoints = meshPoints.size();
        if (numMeshPoints > 0)
        {
            AZ::Vector3 boneCenter = nodeTransform.GetTranslation() + 0.5f * boneDirection;
            float sumDistanceFromAxisSq = 0.0f;
            float boneLengthSqReciprocal = 1.0f / (boneLength * boneLength);
            for (int i = 0; i < numMeshPoints; i++)
            {
                meshPoints[i] -= boneCenter;
                AZ::Vector3 parallelComponent = boneLengthSqReciprocal * meshPoints[i].Dot(boneDirection) * boneDirection;
                meshPoints[i] -= parallelComponent;
                float distanceSq = meshPoints[i].GetLengthSq();
                sumDistanceFromAxisSq += distanceSq;
            }
            rootMeanSquareDistanceFromBone = std::sqrt(sumDistanceFromAxisSq / numMeshPoints);
        }

        // set the position for the collider at the center of the bone
        AZ::Vector3 localBoneDirection = nodeTransformInverse.TransformVector(boneDirection);
        collider.first->m_position = 0.5f * localBoneDirection;

        const AZ::TypeId colliderType = collider.second->RTTI_GetType();
        if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
        {
            Physics::SphereShapeConfiguration* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());
            sphere->m_radius = AZ::GetMin(0.5f * boneLength, rootMeanSquareDistanceFromBone);
        }
        else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
        {
            Physics::CapsuleShapeConfiguration* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());
            capsule->m_height = boneDirection.GetLength();
            if (!localBoneDirection.IsZero())
            {
                collider.first->m_rotation = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), localBoneDirection.GetNormalized());
            }
            capsule->m_height = boneLength;
            const float radius = AZ::GetMin(rootMeanSquareDistanceFromBone, minRadiusRatio * boneLength);
            capsule->m_radius = radius;
        }
        else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
        {
            if (!localBoneDirection.IsZero())
            {
                collider.first->m_rotation = AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), localBoneDirection.GetNormalized());
            }
            Physics::BoxShapeConfiguration* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());
            box->m_dimensions = AZ::Vector3(2.0f * rootMeanSquareDistanceFromBone, 2.0f * rootMeanSquareDistanceFromBone, boneLength);
        }
    }

    void PhysicsSetup::OptimizeForServer()
    {
        // The server only need hit detection colliders. We can remove all the other collider setup.
        m_config.m_clothConfig.m_nodes.clear();
        m_config.m_ragdollConfig.m_nodes.clear();
        m_config.m_simulatedObjectColliderConfig.m_nodes.clear();
    }

    void PhysicsSetup::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysicsSetup>()
                ->Version(4, &VersionConverter)
                ->Field("config", &PhysicsSetup::m_config)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PhysicsSetup>("PhysicsSetup", "Physics setup properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    bool PhysicsSetup::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() == 2)
        {
            classElement.RemoveElementByName(AZ_CRC("ragdoll", 0x74a28c4f));
        }

        // Convert legacy hit detection colliders to shape configurations in the animation config from AzFramework.
        if (classElement.GetVersion() < 4 && classElement.FindSubElement(AZ_CRC("hitDetectionColliders", 0x8675a818)))
        {
            AZ::SerializeContext::DataElementNode* animationConfigElement = classElement.FindSubElement(AZ_CRC("config", 0xd48a2f7c));
            if (!animationConfigElement)
            {
                // Create animation config in case it didn't exist before.
                classElement.AddElement(context, "config", azrtti_typeid<Physics::AnimationConfiguration>());
                animationConfigElement = classElement.FindSubElement(AZ_CRC("config", 0xd48a2f7c));
            }

            AZ::SerializeContext::DataElementNode* hitDetectionConfigElement = animationConfigElement->FindSubElement(AZ_CRC("hitDetectionConfig", 0xf55ba0c6));
            if (!hitDetectionConfigElement)
            {
                // Create hit detection config in case it didn't exist before.
                animationConfigElement->AddElement(context, "hitDetectionConfig", azrtti_typeid<Physics::CharacterColliderConfiguration>());
                hitDetectionConfigElement = animationConfigElement->FindSubElement(AZ_CRC("hitDetectionConfig", 0xf55ba0c6));
            }

            Physics::CharacterColliderConfiguration hitDetectionConfig;
            if (!hitDetectionConfigElement->GetData<Physics::CharacterColliderConfiguration>(hitDetectionConfig))
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode* oldColliderSetElement = classElement.FindSubElement(AZ_CRC("hitDetectionColliders", 0x8675a818));
            if (!oldColliderSetElement)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode* oldCollidersElement = oldColliderSetElement->FindSubElement(AZ_CRC("colliders", 0x0373b539));
            if (oldCollidersElement)
            {
                const int numColliders = oldCollidersElement->GetNumSubElements();
                for (int i = 0; i < numColliders; ++i)
                {
                    AZ::SerializeContext::DataElementNode& oldColliderPair = oldCollidersElement->GetSubElement(i);
                    AZ::SerializeContext::DataElementNode& stringElement = oldColliderPair.GetSubElement(0);
                    AZ::SerializeContext::DataElementNode& colliderElement = oldColliderPair.GetSubElement(1);

                    AZStd::string nodeName;
                    if (!stringElement.GetData<AZStd::string>(nodeName))
                    {
                        return false;
                    }

                    AZ::Vector3 position = AZ::Vector3::CreateZero();
                    AZ::Quaternion rotation = AZ::Quaternion::CreateIdentity();
                    AZ::SerializeContext::DataElementNode* colliderBaseElement = colliderElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
                    if (colliderBaseElement)
                    {
                        colliderBaseElement->FindSubElementAndGetData(AZ_CRC("position", 0x462ce4f5), position);
                        colliderBaseElement->FindSubElementAndGetData(AZ_CRC("rotation", 0x297c98f1), rotation);
                    }

                    Physics::CharacterColliderNodeConfiguration* nodeConfig = hitDetectionConfig.FindNodeConfigByName(nodeName);
                    if (!nodeConfig)
                    {
                        Physics::CharacterColliderNodeConfiguration newNodeConfig;
                        newNodeConfig.m_name = nodeName;

                        hitDetectionConfig.m_nodes.emplace_back(newNodeConfig);
                        nodeConfig = &hitDetectionConfig.m_nodes.back();
                    }

                    AzPhysics::ShapeColliderPairList& collisionShapes = nodeConfig->m_shapes;

                    Physics::ColliderConfiguration* colliderConfig = aznew Physics::ColliderConfiguration();
                    colliderConfig->m_position = position;
                    colliderConfig->m_rotation = rotation;

                    static const AZ::TypeId colliderBoxTypeId = "{2325A19D-E286-4A6D-BE94-1F721BFA8C65}";
                    static const AZ::TypeId colliderCapsuleTypeId = "{A52D164D-4834-49DF-AE53-430E0FC55127}";
                    static const AZ::TypeId colliderSphereTypeId = "{5A6CEB6A-0B04-4AE8-BB35-AB0262908A4D}";

                    if (colliderElement.GetId() == colliderBoxTypeId)
                    {
                        Physics::BoxShapeConfiguration* boxShape = aznew Physics::BoxShapeConfiguration();

                        colliderElement.FindSubElementAndGetData(AZ_CRC("Dimensions", 0xe27d8ba5), boxShape->m_dimensions);

                        collisionShapes.emplace_back(AZStd::pair<AZStd::shared_ptr<Physics::ColliderConfiguration>,
                            AZStd::shared_ptr<Physics::ShapeConfiguration> >(colliderConfig, boxShape));
                    }
                    else if (colliderElement.GetId() == colliderCapsuleTypeId)
                    {
                        Physics::CapsuleShapeConfiguration* capsuleShape = aznew Physics::CapsuleShapeConfiguration();

                        colliderElement.FindSubElementAndGetData(AZ_CRC("radius", 0x3b7c6e5a), capsuleShape->m_radius);
                        colliderElement.FindSubElementAndGetData(AZ_CRC("height", 0xf54de50f), capsuleShape->m_height);

                        collisionShapes.emplace_back(AZStd::pair<AZStd::shared_ptr<Physics::ColliderConfiguration>,
                            AZStd::shared_ptr<Physics::ShapeConfiguration> >(colliderConfig, capsuleShape));
                    }
                    else if (colliderElement.GetId() == colliderSphereTypeId)
                    {
                        Physics::SphereShapeConfiguration* sphereShape = aznew Physics::SphereShapeConfiguration();

                        colliderElement.FindSubElementAndGetData(AZ_CRC("radius", 0x3b7c6e5a), sphereShape->m_radius);

                        collisionShapes.emplace_back(AZStd::pair<AZStd::shared_ptr<Physics::ColliderConfiguration>,
                            AZStd::shared_ptr<Physics::ShapeConfiguration> >(colliderConfig, sphereShape));
                    }
                    else
                    {
                        AZ_Error("EMotionFX", false, "Unknown collider type.");
                        delete colliderConfig;
                        return false;
                    }
                }
            }

            if (!classElement.RemoveElementByName(AZ_CRC("hitDetectionColliders", 0x8675a818)))
            {
                return false;
            }

            // Remove the hit detection config in case one was present before converting (All colliders are still there in the ported version).
            AZ::SerializeContext::DataElementNode* newcharacterConfigElement = classElement.FindSubElement(AZ_CRC("config", 0xd48a2f7c));
            if (!newcharacterConfigElement->RemoveElementByName(AZ_CRC("hitDetectionConfig", 0xf55ba0c6)))
            {
                return false;
            }
            newcharacterConfigElement->AddElementWithData(context, "hitDetectionConfig", hitDetectionConfig);
        }

        return true;
    }

    AZ::Vector3 GetBoneDirection(const Skeleton* skeleton, const Node* node)
    {
        AZ::Vector3 boneDirection = AZ::Vector3::CreateAxisX();

        const Pose* bindPose = skeleton->GetBindPose();
        const Transform& nodeBindTransform = bindPose->GetModelSpaceTransform(node->GetNodeIndex());
        const Transform& parentBindTransform = node->GetParentNode()
            ? bindPose->GetModelSpaceTransform(node->GetParentIndex())
            : Transform::CreateIdentity();

        // if there are child nodes, point the bone direction to the average of their positions
        const size_t numChildNodes = node->GetNumChildNodes();
        if (numChildNodes > 0)
        {
            AZ::Vector3 meanChildPosition = AZ::Vector3::CreateZero();

            // weight by the number of descendants of each child node, so that things like jiggle bones and twist bones
            // have little influence on the bone direction.
            float totalSubChildren = 0.0f;
            for (size_t childNumber = 0; childNumber < numChildNodes; childNumber++)
            {
                const size_t childIndex = node->GetChildIndex(childNumber);
                const Node* childNode = skeleton->GetNode(childIndex);
                const float numSubChildren = static_cast<float>(1 + childNode->GetNumChildNodesRecursive());
                totalSubChildren += numSubChildren;
                meanChildPosition += numSubChildren * (bindPose->GetModelSpaceTransform(childIndex).m_position);
            }

            boneDirection = meanChildPosition / totalSubChildren - nodeBindTransform.m_position;
        }
        // otherwise, point the bone direction away from the parent
        else
        {
            boneDirection = nodeBindTransform.m_position - parentBindTransform.m_position;
        }

        return boneDirection;
    }
} // namespace EMotionFX
