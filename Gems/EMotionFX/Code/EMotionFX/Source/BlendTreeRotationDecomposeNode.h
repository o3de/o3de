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
    class EMFX_API BlendTreeRotationDecomposeNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRotationDecomposeNode, "{0C1A1B69-A8A4-4C40-B07B-202CA34BAB82}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_ROTATION  = 0,
            OUTPUTPORT_PITCH    = 0,
            OUTPUTPORT_YAW      = 1,
            OUTPUTPORT_ROLL     = 2,
            OUTPUTPORT_AXIS     = 0,
            OUTPUTPORT_ANGLE    = 1,
        };

        enum
        {
            PORTID_INPUT_ROTATION = 0,
            PORTID_OUTPUT_PITCH   = 0,
            PORTID_OUTPUT_YAW     = 1,
            PORTID_OUTPUT_ROLL    = 2,
            PORTID_OUTPUT_AXIS    = 3,
            PORTID_OUTPUT_ANGLE   = 4,
        };

        enum class DecomposeMode
        {
            Euler,
            AxisAngle,
        };

        BlendTreeRotationDecomposeNode();
        ~BlendTreeRotationDecomposeNode();

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
        DecomposeMode m_decomposeMode = DecomposeMode::Euler;
    };
} // namespace EMotionFX
