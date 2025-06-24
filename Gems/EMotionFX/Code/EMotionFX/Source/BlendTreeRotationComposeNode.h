/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    class EMFX_API BlendTreeRotationComposeNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRotationComposeNode, "{12A80FF9-DC71-46D4-8E3A-FC93CD1D5E58}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_PITCH     = 0,
            INPUTPORT_YAW       = 1,
            INPUTPORT_ROLL      = 2,
            INPUTPORT_AXIS      = 0,
            INPUTPORT_ANGLE     = 1,
            OUTPUTPORT_ROTATION = 0
        };

        enum
        {
            PORTID_INPUT_PITCH      = 0,
            PORTID_INPUT_YAW        = 1,
            PORTID_INPUT_ROLL       = 2,
            PORTID_INPUT_AXIS       = 3,
            PORTID_INPUT_ANGLE      = 4,
            PORTID_OUTPUT_ROTATION  = 0
        };

        enum class ComposeMode
        {
            Euler,
            AxisAngle,
        };

        BlendTreeRotationComposeNode();
        ~BlendTreeRotationComposeNode();

        void Reinit() override;

        bool InitAfterLoading(AnimGraph* animGraph) override;
        
        AZ::Color GetVisualColor() const override          { return AZ::Color(0.5f, 1.0f, 0.5f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void UpdateOutputPortValues(AnimGraphInstance* animGraphInstance);
        ComposeMode m_composeMode = ComposeMode::Euler;
    };
} // namespace EMotionFX
