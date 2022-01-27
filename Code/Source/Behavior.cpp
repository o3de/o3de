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

#include <EMotionFX/Source/ActorInstance.h>
#include <Allocators.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <Behavior.h>
#include <BehaviorInstance.h>
#include <FrameData.h>
#include <FrameDatabase.h>
#include <EMotionFX/Source/TransformData.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        AZ_CLASS_ALLOCATOR_IMPL(Behavior, MotionMatchAllocator, 0)

        bool Behavior::Init(const InitSettings& settings)
        {
            // Import all motion frames.
            size_t totalNumFramesImported = 0;
            size_t totalNumFramesDiscarded = 0;
            for (Motion* motion : settings.m_motionList)
            {
                size_t numFrames = 0;
                size_t numDiscarded = 0;
                std::tie(numFrames, numDiscarded) = m_data.ImportFrames(motion, settings.m_frameImportSettings, false);
                totalNumFramesImported += numFrames;
                totalNumFramesDiscarded += numDiscarded;

                if (settings.m_importMirrored)
                {
                    std::tie(numFrames, numDiscarded) = m_data.ImportFrames(motion, settings.m_frameImportSettings, true);
                    totalNumFramesImported += numFrames;
                    totalNumFramesDiscarded += numDiscarded;
                }

                //AZ_TracePrintf("EMotionFX", "Motion matching behavior '%s' has imported %d frames from motion '%s'", RTTI_GetTypeName(), numFrames, motion->GetName());
            }

            if (totalNumFramesImported > 0 || totalNumFramesDiscarded > 0)
            {
                AZ_TracePrintf("EMotionFX", "Motion matching behavior '%s' has imported a total of %d frames (%d frames discarded) across %d motions. This is %.2f seconds (%.2f minutes) of motion data.", RTTI_GetTypeName(), totalNumFramesImported, totalNumFramesDiscarded, settings.m_motionList.size(), totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate, (totalNumFramesImported / (float)settings.m_frameImportSettings.m_sampleRate) / 60.0f);
            }

            // Register the dynamic parameters that this behavior exposes to the user.
            if (!RegisterParameters(settings))
            {
                AZ_Error("EMotionFX", false, "Failed to register parameters inside motion matching behavior.");
                return false;
            }

            // Register the required types of frame data.
            if (!RegisterFrameDatas(settings))
            {
                AZ_Error("EMotionFX", false, "Failed to register frame datas inside motion matching behavior.");
                return false;
            }

            // Now build the per frame data (slow).
            if (!m_data.GenerateFrameDatas(settings.m_actorInstance, settings.m_maxKdTreeDepth, settings.m_minFramesPerKdTreeNode))
            {
                AZ_Error("EMotionFX", false, "Failed to generate frame datas inside motion matching behavior.");
                return false;
            }

            return true;
        }


        Behavior* Behavior::CreateBehaviorByType(const AZ::TypeId& typeId)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!context)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return nullptr;
            }

            const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeId);
            if (!classData)
            {
                AZ_Warning("EMotionFX", false, "Can't find class data for this type.");
                return nullptr;
            }

            Behavior* behavior = reinterpret_cast<Behavior*>(classData->m_factory->Create(classData->m_name));
            return behavior;
        }

        void Behavior::DebugDraw(EMotionFX::DebugDraw::ActorInstanceData& draw, BehaviorInstance* behaviorInstance)
        {
            m_data.DebugDraw(draw, behaviorInstance);
        }

        void Behavior::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (!serializeContext)
            {
                return;
            }

            serializeContext->Class<Behavior>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (!editContext)
            {
                return;
            }

            editContext->Class<Behavior>("MotionMatchBehavior", "Base class for motion matching behaviors")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    } // namespace MotionMatching
} // namespace EMotionFX
