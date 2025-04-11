/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <MCore/Source/Attribute.h>
#include <MCore/Source/MCoreSystem.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Importer/SharedFileFormatStructs.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AttributePose, MCore::AttributeAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AttributeMotionInstance, MCore::AttributeAllocator)

    // static create
    AttributePose* AttributePose::Create()
    {
        return aznew AttributePose();
    }


    AttributePose* AttributePose::Create(AnimGraphPose* pose)
    {
        AttributePose* result = aznew AttributePose();
        result->SetValue(pose);
        return result;
    }


    //---------------------------------------------------------------------------------------------------------------------

    AttributeMotionInstance* AttributeMotionInstance::Create()
    {
        return aznew AttributeMotionInstance();
    }


    AttributeMotionInstance* AttributeMotionInstance::Create(MotionInstance* motionInstance)
    {
        AttributeMotionInstance* result = aznew AttributeMotionInstance();
        result->SetValue(motionInstance);
        return result;
    }

    //---------------------------------------------------------------------------------------------------------------------

    void AnimGraphPropertyUtils::ReinitJointIndices(const Actor* actor, const AZStd::vector<AZStd::string>& jointNames, AZStd::vector<size_t>& outJointIndices)
    {
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t jointCount = jointNames.size();

        outJointIndices.clear();
        outJointIndices.reserve(jointCount);

        for (const AZStd::string& jointName : jointNames)
        {
            const Node* node = skeleton->FindNodeByName(jointName);
            if (node)
            {
                outJointIndices.emplace_back(node->GetNodeIndex());
            }
        }
    }
} // namespace EMotionFX
