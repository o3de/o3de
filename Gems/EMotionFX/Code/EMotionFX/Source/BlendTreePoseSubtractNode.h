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
    // forward declarations
    class AnimGraphPose;
    class AnimGraphInstance;


    class EMFX_API BlendTreePoseSubtractNode
        : public AnimGraphNode
    {
    public:        
        AZ_RTTI(BlendTreePoseSubtractNode, "{2CB9593E-BBE4-48DD-A5AF-6E6659CA7FB9}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE_A    = 0,
            INPUTPORT_POSE_B    = 1,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE_A = 0,
            PORTID_INPUT_POSE_B = 1,
            PORTID_OUTPUT_POSE  = 0
        };

        BlendTreePoseSubtractNode();
        ~BlendTreePoseSubtractNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetHasOutputPose() const override              { return true; }
        bool GetSupportsDisable() const override            { return true; }
        bool GetSupportsVisualization() const override      { return true; }
        AZ::Color GetVisualColor() const override           { return AZ::Color(0.62f, 0.32f, 1.0f, 1.0f); }

        void SetSyncMode(ESyncMode eventMode)               { m_syncMode = eventMode; }
        ESyncMode GetSyncMode() const                       { return m_syncMode; }

        void SetEventMode(EEventMode eventMode)             { m_eventMode = eventMode; }
        EEventMode GetEventMode() const                     { return m_eventMode; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, AnimGraphNodeData* uniqueData);

        ESyncMode   m_syncMode;
        EEventMode  m_eventMode;
    };
} // namespace EMotionFX