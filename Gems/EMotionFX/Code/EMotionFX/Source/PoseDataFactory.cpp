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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/PoseDataFactory.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>


namespace EMotionFX
{
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

    const AZStd::unordered_set<AZ::TypeId>& PoseDataFactory::GetTypeIds()
    {
        static AZStd::unordered_set<AZ::TypeId> typeIds =
        {
            azrtti_typeid<PoseDataRagdoll>()
        };

        return typeIds;
    }
} // namespace EMotionFX