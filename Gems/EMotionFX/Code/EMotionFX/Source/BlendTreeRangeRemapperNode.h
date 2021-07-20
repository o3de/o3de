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
     *
     *
     */
    class EMFX_API BlendTreeRangeRemapperNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRangeRemapperNode, "{D60E6686-ECBF-4B8F-A5A5-1164EE66C248}", AnimGraphNode);
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_X         = 0,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_OUTPUT_RESULT    = 1
        };

        BlendTreeRangeRemapperNode();
        ~BlendTreeRangeRemapperNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override               { return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f); }
        bool GetSupportsDisable() const override                { return true; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetInputMin(float value);
        void SetInputMax(float value);
        void SetOutputMin(float value);
        void SetOutputMax(float value);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        float m_inputMin;
        float m_inputMax;
        float m_outputMin;
        float m_outputMax;
    };
} // namespace EMotionFX
