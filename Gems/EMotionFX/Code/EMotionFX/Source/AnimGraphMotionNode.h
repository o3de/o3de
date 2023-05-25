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
#include "AnimGraphNodeData.h"
#include "PlayBackInfo.h"

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class Motion;
    class ActorInstance;
    class MotionSet;

    /**
     *
     *
     */
    class EMFX_API AnimGraphMotionNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphMotionNode, "{B8B8AAE6-E532-4BF8-898F-3D40AA41BC82}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum : uint16
        {
            INPUTPORT_PLAYSPEED                 = 0,
            INPUTPORT_INPLACE                   = 1,

            OUTPUTPORT_POSE                     = 0,
            OUTPUTPORT_MOTION                   = 1
        };

        enum : uint16
        {
            PORTID_INPUT_PLAYSPEED              = 0,
            PORTID_INPUT_INPLACE                = 1,

            PORTID_OUTPUT_POSE                  = 0,
            PORTID_OUTPUT_MOTION                = 1
        };

        enum EIndexMode : AZ::u8
        {
            INDEXMODE_RANDOMIZE                 = 0,
            INDEXMODE_RANDOMIZE_NOREPEAT        = 1,
            INDEXMODE_SEQUENTIAL                = 2,
            INDEXMODE_NUMMETHODS
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() override;

            void Reset() override;
            void Update() override;

        public:
            uint32 m_motionSetId = InvalidIndex32;
            uint32 m_activeMotionIndex = InvalidIndex32;
            MotionInstance* m_motionInstance = nullptr;
            bool m_reload = false;
        };

        AnimGraphMotionNode();
        ~AnimGraphMotionNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        bool GetHasOutputPose() const override { return true; }
        bool GetCanActAsState() const override { return true; }
        bool GetSupportsDisable() const override { return true; }
        bool GetSupportsVisualization() const override { return true; }
        bool GetSupportsPreviewMotion() const override { return true; }
        bool GetNeedsNetTimeSync() const override { return true; }
        AZ::Color GetVisualColor() const override { return AZ::Color(0.38f, 0.24f, 0.91f, 1.0f); }

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void OnActorMotionExtractionNodeChanged() override;
        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        MotionInstance* FindMotionInstance(AnimGraphInstance* animGraphInstance) const;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void UpdatePlayBackInfo(AnimGraphInstance* animGraphInstance);

        float ExtractCustomPlaySpeed(AnimGraphInstance* animGraphInstance) const;
        void PickNewActiveMotion(AnimGraphInstance* animGraphInstance);
        void PickNewActiveMotion(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);

        size_t GetNumMotions() const;
        const char* GetMotionId(size_t index) const;
        void ReplaceMotionId(const char* what, const char* replaceWith);
        void AddMotionId(const AZStd::string& name);

        float GetMotionPlaySpeed() const            { return m_playSpeed; }
        bool GetIsLooping() const                   { return m_loop; }
        bool GetIsRetargeting() const               { return m_retarget; }
        bool GetIsReversed() const                  { return m_reverse; }
        bool GetEmitEvents() const                  { return m_emitEvents; }
        bool GetMirrorMotion() const                { return m_mirrorMotion; }
        bool GetIsMotionExtraction() const          { return m_motionExtraction; }
        float GetDefaultPlaySpeed() const { return m_playSpeed; }

        void SetMotionIds(const AZStd::vector<AZStd::string>& motionIds);
        void SetLoop(bool loop);
        void SetRetarget(bool retarget);
        void SetReverse(bool reverse);
        void SetEmitEvents(bool emitEvents);
        void SetMirrorMotion(bool mirrorMotion);
        void SetMotionExtraction(bool motionExtraction);
        void SetMotionPlaySpeed(float playSpeed);
        void SetIndexMode(EIndexMode eIndexMode);
        void SetNextMotionAfterLoop(bool nextMotionAfterLoop);
        void SetRewindOnZeroWeight(bool rewindOnZeroWeight);
        int FindCumulativeProbabilityIndex(float randomValue) const;

        bool GetIsInPlace(AnimGraphInstance* animGraphInstance) const;

        static void Reflect(AZ::ReflectContext* context);
        static void InitializeDefaultMotionIdsRandomWeights(const AZStd::vector<AZStd::string>& motionIds, AZStd::vector<AZStd::pair<AZStd::string, float> >& motionIdsRandomWeights);

    private:
        static const float                                  s_defaultWeight;

        PlayBackInfo                                        m_playInfo;
        AZStd::vector<AZStd::pair<AZStd::string, float> >    m_motionRandomSelectionCumulativeWeights;
        float                                               m_playSpeed;
        EIndexMode                                          m_indexMode;
        bool                                                m_loop;
        bool                                                m_retarget;
        bool                                                m_reverse;
        bool                                                m_emitEvents;
        bool                                                m_mirrorMotion;
        bool                                                m_motionExtraction;
        bool                                                m_nextMotionAfterLoop;
        bool                                                m_rewindOnZeroWeight;
        bool                                                m_inPlace;

        void ReloadAndInvalidateUniqueDatas();
        MotionInstance* CreateMotionInstance(ActorInstance* actorInstance, UniqueData* uniqueData);
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        void OnMotionIdsChanged();

        AZ::Crc32 GetRewindOnZeroWeightVisibility() const;
        AZ::Crc32 GetMultiMotionWidgetsVisibility() const;
        void SelectAnyRandomMotionIndex(EMotionFX::AnimGraphInstance* animGraphInstance, EMotionFX::AnimGraphMotionNode::UniqueData* uniqueData);

        void UpdateNodeInfo();
    };
} // namespace EMotionFX
