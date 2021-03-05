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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphPlayTimeCondition
        : public AnimGraphTransitionCondition
    {
    public:
        AZ_RTTI(AnimGraphPlayTimeCondition, "{5368D058-9552-4282-A273-AA9344E65D2E}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        enum Mode : AZ::u8
        {
            MODE_REACHEDTIME    = 0,
            MODE_REACHEDEND     = 1,
            MODE_HASLESSTHAN    = 2
        };

        AnimGraphPlayTimeCondition();
        AnimGraphPlayTimeCondition(AnimGraph* animGraph);
        ~AnimGraphPlayTimeCondition();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        void SetNodeId(AnimGraphNodeId nodeId);
        AnimGraphNodeId GetNodeId() const;
        AnimGraphNode* GetNode() const;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        void SetPlayTime(float playTime);
        float GetPlayTime() const;

        void SetMode(Mode mode);
        Mode GetMode() const;
        const char* GetModeString() const;

        void GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::Crc32 GetModeVisibility() const;
        AZ::Crc32 GetPlayTimeVisibility() const;

        static const char* s_modeReachedPlayTimeX;
        static const char* s_modeReachedEnd;
        static const char* s_modeHasLessThanXSecondsLeft;

        AnimGraphNode*      m_node;
        AZ::u64             m_nodeId;
        float               m_playTime;
        Mode                m_mode;
    };
} // namespace EMotionFX
