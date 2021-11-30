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
    class EMFX_API BlendTreeMaskLegacyNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeMaskLegacyNode, "{24647B8B-05B4-4D5D-9161-F0AD0B456B09}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_POSE_0    = 0,
            INPUTPORT_POSE_1    = 1,
            INPUTPORT_POSE_2    = 2,
            INPUTPORT_POSE_3    = 3,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_POSE_0     = 0,
            PORTID_INPUT_POSE_1     = 1,
            PORTID_INPUT_POSE_2     = 2,
            PORTID_INPUT_POSE_3     = 3,
            PORTID_OUTPUT_RESULT    = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() = default;

            void Update() override;

        public:
            AZStd::vector< AZStd::vector<size_t> > m_masks;
        };

        BlendTreeMaskLegacyNode();
        ~BlendTreeMaskLegacyNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool GetHasOutputPose() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }
        AZ::Color GetVisualColor() const override                   { return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override         { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        bool GetOutputEvents(size_t index) const;

        void SetMask0(const AZStd::vector<AZStd::string>& mask0);
        void SetMask1(const AZStd::vector<AZStd::string>& mask1);
        void SetMask2(const AZStd::vector<AZStd::string>& mask2);
        void SetMask3(const AZStd::vector<AZStd::string>& mask3);

        static size_t GetNumMasks() { return s_numMasks; }
        const AZStd::vector<AZStd::string>& GetMask0() const { return m_mask0; }
        const AZStd::vector<AZStd::string>& GetMask1() const { return m_mask1; }
        const AZStd::vector<AZStd::string>& GetMask2() const { return m_mask2; }
        const AZStd::vector<AZStd::string>& GetMask3() const { return m_mask3; }

        void SetOutputEvents0(bool outputEvents0);
        void SetOutputEvents1(bool outputEvents1);
        void SetOutputEvents2(bool outputEvents2);
        void SetOutputEvents3(bool outputEvents3);

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        AZStd::string GetMask0JointName(int index) const;
        AZStd::string GetMask1JointName(int index) const;
        AZStd::string GetMask2JointName(int index) const;
        AZStd::string GetMask3JointName(int index) const;

        AZStd::vector<AZStd::string>        m_mask0;
        AZStd::vector<AZStd::string>        m_mask1;
        AZStd::vector<AZStd::string>        m_mask2;
        AZStd::vector<AZStd::string>        m_mask3;
        bool                                m_outputEvents0;
        bool                                m_outputEvents1;
        bool                                m_outputEvents2;
        bool                                m_outputEvents3;

        static size_t                       s_numMasks;
    };
}   // namespace EMotionFX
