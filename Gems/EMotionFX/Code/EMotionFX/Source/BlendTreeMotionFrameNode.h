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
#include "AnimGraphNodeData.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API BlendTreeMotionFrameNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeMotionFrameNode, "{37B59DF1-496E-453C-91F3-D51821CC3919}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_MOTION        = 0,
            INPUTPORT_TIME          = 1,
            OUTPUTPORT_RESULT       = 0
        };

        enum
        {
            PORTID_INPUT_MOTION     = 0,
            PORTID_INPUT_TIME       = 1,
            PORTID_OUTPUT_RESULT    = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                : AnimGraphNodeData(node, animGraphInstance) { Reset(); }
            ~UniqueData() {}

            void Reset() override
            {
                mOldTime = 0.0f;
                mNewTime = 0.0f;
                m_rewindRequested = false;
            }

        public:
            float   mOldTime;
            float   mNewTime;
            bool    m_rewindRequested;
        };

        BlendTreeMotionFrameNode();
        ~BlendTreeMotionFrameNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetHasOutputPose() const override                                                  { return true; }
        bool GetSupportsVisualization() const override                                          { return true; }
        AZ::Color GetVisualColor() const override                                               { return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }
        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

        void SetNormalizedTimeValue(float value);
        float GetNormalizedTimeValue() const;

        void SetEmitEventsFromStart(bool emitEventsFromStart);
        bool GetEmitEventsFromStart() const;

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        float m_normalizedTimeValue;
        bool m_emitEventsFromStart;
    };
} // namespace EMotionFX
