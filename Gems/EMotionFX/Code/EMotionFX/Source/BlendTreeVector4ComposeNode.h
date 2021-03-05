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
    class EMFX_API BlendTreeVector4ComposeNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeVector4ComposeNode, "{D9C297CE-88BC-47A7-B897-C4B194251E95}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            INPUTPORT_Z         = 2,
            INPUTPORT_W         = 3,
            OUTPUTPORT_VECTOR   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_INPUT_Y          = 1,
            PORTID_INPUT_Z          = 2,
            PORTID_INPUT_W          = 3,
            PORTID_OUTPUT_VECTOR    = 0
        };

        BlendTreeVector4ComposeNode();
        ~BlendTreeVector4ComposeNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override          { return AZ::Color(0.5f, 1.0f, 0.5f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void UpdateOutputPortValues(AnimGraphInstance* animGraphInstance);
    };
} // namespace EMotionFX
