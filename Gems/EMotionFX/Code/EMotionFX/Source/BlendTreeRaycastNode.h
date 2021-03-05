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

#include <AzCore/Math/Vector3.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    class EMFX_API BlendTreeRaycastNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRaycastNode, "{0725660F-3A3D-431F-970A-07D2EB5BB06D}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_RAY_START         = 0,
            INPUTPORT_RAY_END           = 1,
            OUTPUTPORT_POSITION         = 0,
            OUTPUTPORT_NORMAL           = 1,
            OUTPUTPORT_INTERSECTED      = 2
        };

        enum
        {
            PORTID_INPUT_RAY_START      = 0,
            PORTID_INPUT_RAY_END        = 1,
            PORTID_OUTPUT_POSITION      = 0,
            PORTID_OUTPUT_NORMAL        = 1,
            PORTID_OUTPUT_INTERSECTED   = 2
        };

        BlendTreeRaycastNode();
        ~BlendTreeRaycastNode() override;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override      { return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void DoOutput(AnimGraphInstance* animGraphInstance);
    };
}   // namespace EMotionFX
