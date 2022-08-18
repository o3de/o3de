/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/Ragdoll/PhysicsPicking.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <Editor/SkeletonModel.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <EMotionFX/Source/TransformData.h>
#include <AzCore/Math/IntersectSegment.h>

namespace EMotionFX
{
    struct PickingIntersection
    {
        bool m_intersected = false;
        float m_distance = AZ::Constants::FloatMax;
        size_t m_jointIndex = 0;
    };

    bool PhysicsPicking::HandleMouseInteraction(
        [[maybe_unused]] const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteractionEvent)
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

        ActorInstance* actorInstance = skeletonModel->GetActorInstance();
        if (!actorInstance)
        {
            return false;
        }

        PickingIntersection closestIntersection;
        const EMotionFX::Skeleton* skeleton = actorInstance->GetActor()->GetSkeleton();

        if (AZ::RHI::CheckBitsAny(m_renderFlags, EMotionFX::ActorRenderFlags::LineSkeleton))
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
                int numIntersections = AZ::Intersect::IntersectRayCappedCylinder(
                    mouseInteraction.m_mousePick.m_rayOrigin,
                    mouseInteraction.m_mousePick.m_rayDirection,
                    bonePos,
                    boneDir,
                    boneLength,
                    PickingMargin,
                    t1,
                    t2);
                if (numIntersections > 0)
                {
                    float distance = (numIntersections == 1) ? t1 : AZ::GetMin(t1, t2);
                    if (distance < closestIntersection.m_distance)
                    {
                        closestIntersection.m_intersected = true;
                        closestIntersection.m_distance = distance;
                        closestIntersection.m_jointIndex = jointIndex;
                    }
                }
            }
        }

        if (closestIntersection.m_intersected)
        {
            Node* node = skeleton->GetNode(closestIntersection.m_jointIndex);
            if (node)
            {
                QModelIndex modelIndex = skeletonModel->GetModelIndex(node->GetParentNode());
                skeletonModel->GetSelectionModel().select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            }
        }

        return closestIntersection.m_intersected;
    }

    void PhysicsPicking::UpdateRenderFlags(ActorRenderFlags renderFlags)
    {
        m_renderFlags = renderFlags;
    }
} // namespace EMotionFX
