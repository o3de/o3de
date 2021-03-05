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

#pragma once

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeFloatSwitchNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFloatSwitchNode, "{8DDB9197-74A4-4C75-A58F-5B68C924FCF1}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_0         = 0,
            INPUTPORT_1         = 1,
            INPUTPORT_2         = 2,
            INPUTPORT_3         = 3,
            INPUTPORT_4         = 4,
            INPUTPORT_DECISION  = 5,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_0          = 0,
            PORTID_INPUT_1          = 1,
            PORTID_INPUT_2          = 2,
            PORTID_INPUT_3          = 3,
            PORTID_INPUT_4          = 4,
            PORTID_INPUT_DECISION   = 5,
            PORTID_OUTPUT_RESULT    = 0
        };

        BlendTreeFloatSwitchNode();
        ~BlendTreeFloatSwitchNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override;
        float GetValue(uint32 index) const;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetValue0(float value0);
        void SetValue1(float value1);
        void SetValue2(float value2);
        void SetValue3(float value3);
        void SetValue4(float value3);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        float   m_value0;
        float   m_value1;
        float   m_value2;
        float   m_value3;
        float   m_value4;
    };
} // namespace EMotionFX