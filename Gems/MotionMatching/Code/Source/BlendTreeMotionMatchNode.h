/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Timer.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MotionMatchingInstance.h>
#include <FeatureSchema.h>
#include <MotionMatchingData.h>
#include <ImGuiMonitor.h>

namespace EMotionFX::MotionMatching
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
            INPUTPORT_TARGETFACINGDIR = 1,
            INPUTPORT_USEFACINGDIR = 2,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_INPUT_TARGETPOS = 0,
            PORTID_INPUT_TARGETFACINGDIR = 1,
            PORTID_INPUT_USEFACINGDIR = 2,
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
                delete m_data;
                delete m_instance;
            }

            void Update() override;

        public:
            MotionMatching::MotionMatchingInstance* m_instance = nullptr;
            MotionMatching::MotionMatchingData* m_data = nullptr;
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

        const FeatureSchema& GetFeatureSchema() const { return m_featureSchema; }

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        AZ::Crc32 GetTrajectoryPathSettingsVisibility() const;
        AZ::Crc32 GetFeatureScalerTypeSettingsVisibility() const;
        AZ::Crc32 GetMinMaxSettingsVisibility() const;
        AZ::Crc32 OnVisualizeSchemaButtonClicked();
        AZStd::string OnVisualizeSchemaButtonText() const;

        FeatureSchema m_featureSchema;
        AZStd::vector<AZStd::string> m_motionIds;

        float m_pathRadius = 1.0f;
        float m_pathSpeed = 1.0f;
        float m_lowestCostSearchFrequency = 5.0f;
        AZ::u32 m_sampleRate = 30;
        AZ::u32 m_maxKdTreeDepth = 15;
        AZ::u32 m_minFramesPerKdTreeNode = 1000;
        TrajectoryQuery::EMode m_trajectoryQueryMode = TrajectoryQuery::MODE_TARGETDRIVEN;
        bool m_mirror = false;

        // Data normalization.
        bool m_normalizeData = false;
        MotionMatchingData::FeatureScalerType m_featureScalerType = MotionMatchingData::StandardScalerType;
        float m_featureMin = 0.0f;
        float m_featureMax = 1.0f;
        bool m_clipFeatures = false;

        AZ::Debug::Timer m_timer;
        float m_updateTimeInMs = 0.0f;
        float m_postUpdateTimeInMs = 0.0f;
        float m_outputTimeInMs = 0.0f;

#ifdef IMGUI_ENABLED
        ImGuiMonitor m_imguiMonitor;
#endif
    };
} // namespace EMotionFX::MotionMatching
