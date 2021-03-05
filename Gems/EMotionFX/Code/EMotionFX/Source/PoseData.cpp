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

#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseData, PoseAllocator, 0)

    PoseData::PoseData()
        : m_pose(nullptr)
        , m_isUsed(false)
    {
    }

    PoseData::~PoseData()
    {
    }

    void PoseData::LinkToActorInstance(const ActorInstance* actorInstance)
    {
        AZ_UNUSED(actorInstance);
        m_isUsed = false;
    }

    void PoseData::LinkToActor(const Actor* actor)
    {
        AZ_UNUSED(actor);
        m_isUsed = false;
    }

    void PoseData::SetPose(Pose* pose)
    {
        m_pose = pose;
    }

    PoseData& PoseData::operator=(const PoseData& from)
    {
        CopyFrom(&from);
        return *this;
    }

    void PoseData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PoseData>()
                ->Version(1)
                ;
        }
    }
} // namespace EMotionFX
