/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    class EMFX_API BlendTreeVector4DecomposeNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeVector4DecomposeNode, "{1B456B53-F634-40FD-96BA-7590CEAFFCFF}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_VECTOR    = 0,
            OUTPUTPORT_X        = 0,
            OUTPUTPORT_Y        = 1,
            OUTPUTPORT_Z        = 2,
            OUTPUTPORT_W        = 3
        };

        enum
        {
            PORTID_INPUT_VECTOR = 0,
            PORTID_OUTPUT_X     = 0,
            PORTID_OUTPUT_Y     = 1,
            PORTID_OUTPUT_Z     = 2,
            PORTID_OUTPUT_W     = 3
        };

        BlendTreeVector4DecomposeNode();
        ~BlendTreeVector4DecomposeNode();

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
