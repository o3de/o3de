#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            class MotionRangeRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(MotionRangeRule, "{3107B08E-5D9D-49A0-8B1B-2133B5A1B041}", IRule);
                AZ_CLASS_ALLOCATOR_DECL

                MotionRangeRule();
                ~MotionRangeRule() override = default;

                AZ::u32 GetStartFrame() const;
                AZ::u32 GetEndFrame() const;
                bool GetProcessRangeRuleConversion() const;
                void SetStartFrame(AZ::u32 frame);
                void SetEndFrame(AZ::u32 frame);
                void SetProcessRangeRuleConversion(bool processRangeRuleConversion);

                static void Reflect(AZ::ReflectContext* context);

            protected:
                AZ::u32 m_startFrame;
                AZ::u32 m_endFrame;
                bool    m_processRangeRuleConversion;    // if true, this rule is converted from the old data in MotionGroup.
            };
        } // Rule
    } // Pipeline
} // EMotionFX
