/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseData, PoseAllocator)

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
