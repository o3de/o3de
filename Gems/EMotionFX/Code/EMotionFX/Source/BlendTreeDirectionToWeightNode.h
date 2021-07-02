/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    /**
     *
     *
     *
     */
    class EMFX_API BlendTreeDirectionToWeightNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeDirectionToWeightNode, "{05D6BE60-276D-4524-8DCD-79408AAC3398}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_DIRECTION_X   = 0,
            INPUTPORT_DIRECTION_Y   = 1,
            OUTPUTPORT_WEIGHT       = 0
        };

        enum
        {
            PORTID_INPUT_DIRECTION_X = 0,
            PORTID_INPUT_DIRECTION_Y = 1,
            PORTID_OUTPUT_WEIGHT    = 0
        };

        BlendTreeDirectionToWeightNode();
        ~BlendTreeDirectionToWeightNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
