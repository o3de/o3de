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
     * The blend tree's final node.
     * This node always exists inside the blend tree.
     * The input of this node will be what the motion tree's output will be.
     * The final node has only one single input.
     */
    class EMFX_API BlendTreeFinalNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFinalNode, "{1A755218-AD9D-48EA-86FC-D571C11ECA4D}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
        };

        enum
        {
            INPUTPORT_POSE  = 0
        };

        enum
        {
            PORTID_INPUT_POSE   = 0
        };

        BlendTreeFinalNode();
        ~BlendTreeFinalNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override         { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        bool GetHasOutputPose() const override                  { return true; }
        AZ::Color GetVisualColor() const override               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        bool GetIsDeletable() const override                    { return false; }
        bool GetIsLastInstanceDeletable() const override        { return false; }
        bool GetHasVisualOutputPorts() const override           { return false; }
        bool GetCanHaveOnlyOneInsideParent() const override     { return true; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
