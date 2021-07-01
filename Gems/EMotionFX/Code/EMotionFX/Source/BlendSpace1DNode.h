/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <EMotionFX/Source/BlendSpaceNode.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{
    class AnimGraphPose;
    class AnimGraphInstance;
    class MotionInstance;


    class EMFX_API BlendSpace1DNode
        : public BlendSpaceNode
    {
    public:
        AZ_RTTI(BlendSpace1DNode, "{E41B443C-8423-4764-97F0-6C57997C2E5B}", BlendSpaceNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_VALUE = 0,
            INPUTPORT_INPLACE = 1,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_INPUT_VALUE = 0,
            PORTID_INPUT_INPLACE = 1,
            PORTID_OUTPUT_POSE = 0
        };

        struct CurrentSegmentInfo
        {
            AZ::u32 m_segmentIndex;
            float   m_weightForSegmentEnd; // weight for the segment start will be 1 minus this.
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData();

            float GetRangeMin() const;
            float GetRangeMax() const;

            void Reset() override;
            void Update() override;

        public:
            MotionInfos                     m_motionInfos;
            bool                            m_allMotionsHaveSyncTracks;
            AZStd::vector<float>            m_motionCoordinates;
            AZStd::vector<AZ::u16>          m_sortedMotions; // indices to motions sorted according to their parameter values
            float                           m_currentPosition;
            CurrentSegmentInfo              m_currentSegment;
            BlendInfos                      m_blendInfos;
            AZ::u32                         m_leaderMotionIdx; // index of the leader motion for syncing
            bool                            m_hasOverlappingCoordinates; // indicates if two coordinates are overlapping, to notify the UI
        };

        BlendSpace1DNode();
        ~BlendSpace1DNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetValidCalculationMethodAndEvaluator() const;
        const char* GetAxisLabel() const;

        // AnimGraphNode overrides
        bool    GetSupportsVisualization() const override { return true; }
        bool    GetSupportsDisable() const override { return true; }
        bool    GetHasVisualGraph() const override { return true; }
        bool    GetHasOutputPose() const override { return true; }
        bool    GetNeedsNetTimeSync() const override { return true; }
        AZ::Color GetVisualColor() const override { return AZ::Color(0.23f, 0.71f, 0.78f, 1.0f); }

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        // AnimGraphObject overrides
        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        //! Update the positions of all motions in the blend space.
        void UpdateMotionPositions(UniqueData& uniqueData);

        //! Called to set the current position from GUI.
        void SetCurrentPosition(float point);

        void SetCalculationMethod(ECalculationMethod calculationMethod);
        ECalculationMethod GetCalculationMethod() const;

        void SetSyncLeaderMotionId(const AZStd::string& syncLeaderMotionId);
        const AZStd::string& GetSyncLeaderMotionId() const;

        void SetEvaluatorType(const AZ::TypeId& evaluatorType);
        const AZ::TypeId& GetEvaluatorType() const;
        BlendSpaceParamEvaluator* GetEvaluator() const;

        void SetSyncMode(ESyncMode syncMode);
        ESyncMode GetSyncMode() const;

        void SetEventFilterMode(EBlendSpaceEventMode eventFilterMode);
        EBlendSpaceEventMode GetEventFilterMode() const;

        void SetMotions(const AZStd::vector<BlendSpaceMotion>& motions) override;
        const AZStd::vector<BlendSpaceMotion>& GetMotions() const override;

        bool GetIsInPlace(AnimGraphInstance* animGraphInstance) const;

        static void Reflect(AZ::ReflectContext* context);

    public:
        //  BlendSpaceNode overrides

        //! Compute the position of the motion in blend space.
        void ComputeMotionCoordinates(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position) override;

        //! Restore the motion coordinates that are set to automatic mode back to the computed values.
        void RestoreMotionCoordinates(BlendSpaceMotion& motion, AnimGraphInstance* animGraphInstance) override;

    protected:
        // AnimGraphNode overrides
        void Output(AnimGraphInstance* animGraphInstance) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

    private:
        bool UpdateMotionInfos(UniqueData* uniqueData);
        void SortMotionInstances(UniqueData& uniqueData);

        float GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, UniqueData& uniqueData);
        void UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData);
        bool FindLineSegmentForCurrentPoint(UniqueData& uniqueData);
        void SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance);

        static bool NodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool AnimGraphInstanceExists(AnimGraphInstance* animGraphInstance);

    private:
        AZ::Crc32 GetEvaluatorVisibility() const;
        AZ::Crc32 GetSyncOptionsVisibility() const;

        AZStd::vector<BlendSpaceMotion> m_motions;
        AZStd::string                   m_syncLeaderMotionId;
        BlendSpaceParamEvaluator*       m_evaluator = nullptr;
        AZ::TypeId                      m_evaluatorType = azrtti_typeid<BlendSpaceParamEvaluatorNone>();
        ECalculationMethod              m_calculationMethod = ECalculationMethod::AUTO;
        ESyncMode                       m_syncMode = ESyncMode::SYNCMODE_DISABLED;
        EBlendSpaceEventMode            m_eventFilterMode = EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION;
        float                           m_currentPositionSetInteractively = 0.0f;
    };
}   // namespace EMotionFX
