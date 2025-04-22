/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MotionRangeRule, AZ::SystemAllocator)

            MotionRangeRule::MotionRangeRule()
                : m_startFrame(0)
                , m_endFrame(0)
                , m_processRangeRuleConversion(false)
            {
            }

            AZ::u32 MotionRangeRule::GetStartFrame() const
            {
                return m_startFrame;
            }

            AZ::u32 MotionRangeRule::GetEndFrame() const
            {
                return m_endFrame;
            }

            bool MotionRangeRule::GetProcessRangeRuleConversion() const
            {
                return m_processRangeRuleConversion;
            }

            void MotionRangeRule::SetStartFrame(AZ::u32 frame)
            {
                m_startFrame = frame;
            }
            void MotionRangeRule::SetEndFrame(AZ::u32 frame)
            {
                m_endFrame = frame;
            }

            void MotionRangeRule::SetProcessRangeRuleConversion(bool processRangeRuleConversion)
            {
                m_processRangeRuleConversion = processRangeRuleConversion;
            }

            void MotionRangeRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MotionRangeRule, IRule>()->Version(1)
                    ->Field("startFrame", &MotionRangeRule::m_startFrame)
                    ->Field("endFrame", &MotionRangeRule::m_endFrame)
                    ->Field("processRangeRuleConversion", &MotionRangeRule::m_processRangeRuleConversion);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MotionRangeRule>("Motion range", "Define the range of the motion that will be exported.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionRangeRule::m_startFrame, "Start frame", "The start frame of the animation that will be exported.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MotionRangeRule::m_endFrame, "End frame", "The end frame of the animation that will be exported.");
                }
            }
        } // Rule
    } // Pipeline
} // EMotionFX
