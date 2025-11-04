/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(PoseDataFactory, PoseAllocator)

    PoseDataFactory::PoseDataFactory()
    {
        AddPoseDataType(azrtti_typeid<PoseDataRagdoll>());
    }

    PoseData* PoseDataFactory::Create(Pose* pose, const AZ::TypeId& type)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }
        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        
        PoseData* result = static_cast<PoseData*>(classData->m_factory->Create(classData->m_name));
        if (result)
        {
            result->SetPose(pose);
        }
        return result;
    }

    void PoseDataFactory::AddPoseDataType(const AZ::TypeId& poseDataType)
    {
        m_poseDataTypeIds.emplace(poseDataType);
    }

    const AZStd::unordered_set<AZ::TypeId>& PoseDataFactory::GetTypeIds() const
    {
        return m_poseDataTypeIds;
    }
} // namespace EMotionFX
