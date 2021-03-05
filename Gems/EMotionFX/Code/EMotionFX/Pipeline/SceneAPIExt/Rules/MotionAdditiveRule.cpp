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

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/MotionAdditiveRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionAdditiveRule, AZ::SystemAllocator, 0)

             MotionAdditiveRule::MotionAdditiveRule()
                : m_sampleFrameIndex(0)
            {
            }

            size_t MotionAdditiveRule::GetSampleFrameIndex() const
            {
                return m_sampleFrameIndex;
            }

            void MotionAdditiveRule::SetSampleFrameIndex(size_t index)
            {
                m_sampleFrameIndex = index;
            }

            void MotionAdditiveRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MotionAdditiveRule, IRule>()->Version(1)
                    ->Field("sampleFrame", &MotionAdditiveRule::m_sampleFrameIndex);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionAdditiveRule>("Additive motion", "Make the motion an additive motion.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionAdditiveRule::m_sampleFrameIndex, "Base frame" /*sample frame*/, "The frame number that the motion will be made relative to.");
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX