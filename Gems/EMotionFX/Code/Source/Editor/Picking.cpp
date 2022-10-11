/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <Editor/Picking.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <Editor/SkeletonModel.h>

namespace EMotionFX
{
    struct PickingIntersection
    {
        bool m_intersected = false;
        float m_distance = AZ::Constants::FloatMax;
        size_t m_jointIndex = 0;
    };

    void CompareIntersection(PickingIntersection& closestIntersection, float distance, size_t jointIndex)
    {
        if (distance < closestIntersection.m_distance)
        {
            closestIntersection.m_intersected = true;
            closestIntersection.m_distance = distance;
            closestIntersection.m_jointIndex = jointIndex;
        }
    }

    void IntersectLineSkeleton(
        PickingIntersection& closestIntersection,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const ActorInstance* actorInstance,
        const EMotionFX::Skeleton* skeleton)
    {
        const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const size_t lodLevel = actorInstance->GetLODLevel();
        const size_t numJoints = skeleton->GetNumNodes();

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
            const AZ::Vector3 bonePos = pose->GetWorldSpaceTransform(jointIndex).m_position;

            const AZ::Vector3 boneDir = (parentPos - bonePos).GetNormalized();
            const float boneLength = (parentPos - bonePos).GetLength();
            float t1 = AZ::Constants::FloatMax;
            float t2 = AZ::Constants::FloatMax;
            int numIntersections =
                AZ::Intersect::IntersectRayCappedCylinder(rayOrigin, rayDirection, bonePos, boneDir, boneLength, PickingMargin, t1, t2);
            if (numIntersections > 0)
            {
                const float distance = (numIntersections == 1) ? t1 : AZ::GetMin(t1, t2);
                CompareIntersection(closestIntersection, distance, parentIndex);
            }
        }
    }

    void IntersectRagdollColliders(
        PickingIntersection& closestIntersection,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const ActorInstance* actorInstance
    )
    {
        Physics::CharacterColliderConfiguration* ragdollColliderConfiguration =
            actorInstance->GetActor()->GetPhysicsSetup()->GetColliderConfigByType(EMotionFX::PhysicsSetup::Ragdoll);
        for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : ragdollColliderConfiguration->m_nodes)
        {
            const EMotionFX::Actor* actor = actorInstance->GetActor();
            const EMotionFX::Node* joint = actor->GetSkeleton()->FindNodeByName(nodeConfig.m_name.c_str());
            if (joint)
            {
                const size_t jointIndex = joint->GetNodeIndex();
                const EMotionFX::Transform& actorInstanceGlobalTransform = actorInstance->GetWorldSpaceTransform();
                const EMotionFX::Transform& emfxNodeGlobalTransform =
                    actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(jointIndex);
                const AZ::Transform worldTransform = (emfxNodeGlobalTransform * actorInstanceGlobalTransform).ToAZTransform();
                for (const AzPhysics::ShapeColliderPair& shapeColliderPair : nodeConfig.m_shapes)
                {
                    const AZ::Transform colliderOffsetTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
                        shapeColliderPair.first->m_rotation, shapeColliderPair.first->m_position);
                    const AZ::Transform colliderGlobalTransform = worldTransform * colliderOffsetTransform;
                    const AZ::TypeId colliderType = shapeColliderPair.second->RTTI_GetType();
                    if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
                    {
                        auto* sphere = static_cast<Physics::SphereShapeConfiguration*>(shapeColliderPair.second.get());
                        float distance = AZ::Constants::FloatMax;
                        AZ::Intersect::SphereIsectTypes result = AZ::Intersect::IntersectRaySphere(
                            rayOrigin, rayDirection, colliderGlobalTransform.GetTranslation(), sphere->m_radius, distance);
                        if (result != AZ::Intersect::SphereIsectTypes::ISECT_RAY_SPHERE_NONE)
                        {
                            CompareIntersection(closestIntersection, distance, jointIndex);
                        }
                    }
                    else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
                    {
                        auto* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(shapeColliderPair.second.get());
                        const AZ::Vector3 capsuleZ = colliderGlobalTransform.GetBasisZ();
                        const float cylinderHeight = AZ::GetMax(AZ::Constants::Tolerance, capsule->m_height - 2.0f * capsule->m_radius);
                        const AZ::Vector3 cylinderEnd1 = colliderGlobalTransform.GetTranslation() - 0.5f * cylinderHeight * capsuleZ;
                        const AZ::Vector3 cylinderEnd2 = colliderGlobalTransform.GetTranslation() + 0.5f * cylinderHeight * capsuleZ;
                        const float rayLength = 1000.0f;
                        float t = AZ::Constants::FloatMax;
                        const AZ::Intersect::CapsuleIsectTypes result = AZ::Intersect::IntersectSegmentCapsule(
                            rayOrigin, rayDirection * rayLength, cylinderEnd1, cylinderEnd2, capsule->m_radius, t);
                        if (result != AZ::Intersect::CapsuleIsectTypes::ISECT_RAY_CAPSULE_NONE)
                        {
                            const float distance = rayLength * t;
                            CompareIntersection(closestIntersection, distance, jointIndex);
                        }
                    }
                    else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
                    {
                        auto* box = static_cast<Physics::BoxShapeConfiguration*>(shapeColliderPair.second.get());
                        float distance = AZ::Constants::FloatMax;
                        if (AZ::Intersect::IntersectRayBox(
                            rayOrigin,
                            rayDirection,
                            colliderGlobalTransform.GetTranslation(),
                            colliderGlobalTransform.GetBasisX(),
                            colliderGlobalTransform.GetBasisY(),
                            colliderGlobalTransform.GetBasisZ(),
                            0.5f * box->m_dimensions.GetX(),
                            0.5f * box->m_dimensions.GetY(),
                            0.5f * box->m_dimensions.GetZ(),
                            distance))
                        {
                            CompareIntersection(closestIntersection, distance, jointIndex);
                        }
                    }
                }
            }
        }
    }

    bool Picking::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent)
    {
        const auto& mouseInteraction = mouseInteractionEvent.m_mouseInteraction;
        if (!mouseInteraction.m_mouseButtons.Left() ||
            mouseInteractionEvent.m_mouseEvent != AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            return false;
        }

        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (!skeletonModel)
        {
            return false;
        }

        const ActorInstance* actorInstance = skeletonModel->GetActorInstance();
        if (!actorInstance)
        {
            return false;
        }

        PickingIntersection closestIntersection;
        const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();
        const AZ::Vector3& rayOrigin = mouseInteraction.m_mousePick.m_rayOrigin;
        const AZ::Vector3& rayDirection = mouseInteraction.m_mousePick.m_rayDirection;

        if (AZ::RHI::CheckBitsAny(m_renderFlags, EMotionFX::ActorRenderFlags::LineSkeleton))
        {
            IntersectLineSkeleton(closestIntersection, rayOrigin, rayDirection, actorInstance, skeleton);
        }

        if (AZ::RHI::CheckBitsAny(m_renderFlags, EMotionFX::ActorRenderFlags::RagdollColliders))
        {
            IntersectRagdollColliders(closestIntersection, rayOrigin, rayDirection, actorInstance);
        }

        if (closestIntersection.m_intersected)
        {
            QModelIndex modelIndex = skeletonModel->GetModelIndex(skeleton->GetNode(closestIntersection.m_jointIndex));
            skeletonModel->GetSelectionModel().select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }

        return closestIntersection.m_intersected;
    }

    void Picking::SetRenderFlags(ActorRenderFlags renderFlags)
    {
        m_renderFlags = renderFlags;
    }
} // namespace EMotionFX
