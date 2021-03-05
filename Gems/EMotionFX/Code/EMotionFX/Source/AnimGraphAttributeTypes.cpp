/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    AZ_CLASS_ALLOCATOR_IMPL(AttributePose, MCore::AttributeAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AttributeMotionInstance, MCore::AttributeAllocator, 0)

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

    void AnimGraphPropertyUtils::ReinitJointIndices(const Actor* actor, const AZStd::vector<AZStd::string>& jointNames, AZStd::vector<AZ::u32>& outJointIndices)
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