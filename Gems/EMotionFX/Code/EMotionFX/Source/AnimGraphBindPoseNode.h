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
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphBindPoseNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphBindPoseNode, "{72595B5C-045C-4DB1-88A4-40BC4560D7AF}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum : uint16
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum : uint16
        {
            PORTID_OUTPUT_POSE = 0
        };

        AnimGraphBindPoseNode();
        ~AnimGraphBindPoseNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override               { return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f); }
        bool GetCanActAsState() const override                  { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
    };
}   // namespace EMotionFX
