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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    class BlendNParamWeight
    {
    public:
        AZ_RTTI(BlendNParamWeight, "{072E5508-B119-41DD-9915-717E750A984B}")
        AZ_CLASS_ALLOCATOR_DECL

        friend class BlendTreeBlendNNode;

        BlendNParamWeight() = default;
        BlendNParamWeight(AZ::u32 portId, float weightRange);
        virtual ~BlendNParamWeight() = default;

        AZ::u32 GetPortId() const;
        float GetWeightRange() const;
        const char* GetPortLabel() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::u32         m_portId = MCORE_INVALIDINDEX32;
        float           m_weightRange = 0;
    };

    class EMFX_API BlendTreeBlendNNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeBlendNNode, "{CBFFDE41-008D-45A1-AC2A-E9A25C8CE62A}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum : uint16
        {
            INPUTPORT_POSE_0    = 0,
            INPUTPORT_POSE_1    = 1,
            INPUTPORT_POSE_2    = 2,
            INPUTPORT_POSE_3    = 3,
            INPUTPORT_POSE_4    = 4,
            INPUTPORT_POSE_5    = 5,
            INPUTPORT_POSE_6    = 6,
            INPUTPORT_POSE_7    = 7,
            INPUTPORT_POSE_8    = 8,
            INPUTPORT_POSE_9    = 9,
            INPUTPORT_WEIGHT    = 10,
            OUTPUTPORT_POSE     = 0
        };

        enum : uint16
        {
            PORTID_INPUT_POSE_0 = 0,
            PORTID_INPUT_POSE_1 = 1,
            PORTID_INPUT_POSE_2 = 2,
            PORTID_INPUT_POSE_3 = 3,
            PORTID_INPUT_POSE_4 = 4,
            PORTID_INPUT_POSE_5 = 5,
            PORTID_INPUT_POSE_6 = 6,
            PORTID_INPUT_POSE_7 = 7,
            PORTID_INPUT_POSE_8 = 8,
            PORTID_INPUT_POSE_9 = 9,
            PORTID_INPUT_WEIGHT = 10,

            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);

            void Update() override;

        public:
            uint32 m_indexA = InvalidIndex32;
            uint32 m_indexB = InvalidIndex32;
        };

        BlendTreeBlendNNode();
        ~BlendTreeBlendNNode() = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        AZ::Color GetVisualColor() const override               { return AZ::Color(0.62f, 0.32f, 1.0f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outNodeA, AnimGraphNode** outNodeB, uint32* outIndexA, uint32* outIndexB, float* outWeight) const;

        static void Reflect(AZ::ReflectContext* context);
        void SetSyncMode(ESyncMode syncMode);
        void SetEventMode(EEventMode eventMode);
        bool HasRequiredInputs() const;

        void UpdateParamWeights();
        void UpdateParamWeightRanges();
        void SetParamWeightsEquallyDistributed(float min, float max);
        const AZStd::vector<BlendNParamWeight>& GetParamWeights();

        static const char* GetPoseInputPortName(AZ::u32 portId);

    private:
        void SyncMotions(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, uint32 poseIndexA, uint32 poseIndexB, float blendWeight, ESyncMode syncMode);
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        ESyncMode                                   m_syncMode;
        EEventMode                                  m_eventMode;
        AZStd::vector<BlendNParamWeight>            m_paramWeights;
    };
} // namespace EMotionFX
