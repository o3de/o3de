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

#include <AzCore/Debug/Timer.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <BehaviorInstance.h>
#include <LocomotionBehavior.h>
#include <ImGuiMonitor.h>

namespace EMotionFX
{
    namespace MotionMatching
    {
        class EMFX_API BlendTreeMotionMatchNode
            : public AnimGraphNode
        {
        public:
            AZ_RTTI(BlendTreeMotionMatchNode, "{1DC80DCD-6536-4950-9260-A4615C03E3C5}", AnimGraphNode)
            AZ_CLASS_ALLOCATOR_DECL

            enum
            {
                INPUTPORT_TARGETPOS = 0,
                OUTPUTPORT_POSE = 0
            };

            enum
            {
                PORTID_INPUT_TARGETPOS = 0,
                PORTID_OUTPUT_POSE = 0
            };

            class EMFX_API UniqueData
                : public AnimGraphNodeData
            {
                EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
            public:
                AZ_CLASS_ALLOCATOR_DECL

                UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
                    : AnimGraphNodeData(node, animGraphInstance)
                {
                }

                ~UniqueData()
                {
                    delete m_behavior;
                    delete m_behaviorInstance;
                }

                void Update() override;

            public:
                MotionMatching::BehaviorInstance* m_behaviorInstance = nullptr;
                MotionMatching::LocomotionBehavior* m_behavior = nullptr;
            };

            BlendTreeMotionMatchNode();
            ~BlendTreeMotionMatchNode();

            bool InitAfterLoading(AnimGraph* animGraph) override;
            
            bool GetSupportsVisualization() const override { return true; }
            bool GetHasOutputPose() const override { return true; }
            bool GetSupportsDisable() const override { return true; }
            AZ::Color GetVisualColor() const override { return AZ::Colors::Green; }
            AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

            const char* GetPaletteName() const override;
            AnimGraphObject::ECategory GetPaletteCategory() const override;

            AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

            static void Reflect(AZ::ReflectContext* context);

        private:
            void Output(AnimGraphInstance* animGraphInstance) override;
            void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
            void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

            AZStd::vector<AZStd::string> m_motionIds;

            float m_footPositionFactor = 1.0f;
            float m_footVelocityFactor = 1.0f;
            float m_rootFutureFactor = 1.0f;
            float m_rootPastFactor = 1.0f;
            float m_differentMotionFactor = 1.0f;
            float m_pathRadius = 1.0f;
            float m_pathSpeed = 1.0f;
            float m_lowestCostSearchFrequency = 0.1f;
            AZ::u32 m_sampleRate = 15;
            AZ::u32 m_maxKdTreeDepth = 15;
            AZ::u32 m_minFramesPerKdTreeNode = 1000;
            EControlSplineMode m_controlSplineMode = MODE_TARGETDRIVEN;
            bool m_mirror = false;

            AZ::Debug::Timer m_timer;
            float m_updateTimeInMs = 0.0f;
            float m_postUpdateTimeInMs = 0.0f;
            float m_outputTimeInMs = 0.0f;

            ImGuiMonitor m_imguiMonitor;
        };
    } // namespace MotionMatching
} // namespace EMotionFX
