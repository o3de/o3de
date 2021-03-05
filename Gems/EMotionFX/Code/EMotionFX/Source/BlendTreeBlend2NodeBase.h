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

#include <AzCore/std/containers/vector.h>


namespace EMotionFX
{

    class EMFX_API BlendTreeBlend2NodeBase
        : public AnimGraphNode
    {
    public:        
        AZ_RTTI(BlendTreeBlend2NodeBase, "{7380C346-7568-42A5-BC1D-486646789717}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        using WeightedMaskEntry = AZStd::pair<AZStd::string, float>;

        enum
        {
            INPUTPORT_POSE_A    = 0,
            INPUTPORT_POSE_B    = 1,
            INPUTPORT_WEIGHT    = 2,
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_INPUT_POSE_A = 0,
            PORTID_INPUT_POSE_B = 1,
            PORTID_INPUT_WEIGHT = 2,
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
            AZStd::vector<uint32>   mMask;
            AnimGraphNode*          mSyncTrackNode;
        };

        BlendTreeBlend2NodeBase();
        virtual ~BlendTreeBlend2NodeBase();

        bool GetHasOutputPose() const override                  { return true; }
        bool GetSupportsDisable() const override                { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        AZ::Color GetVisualColor() const override               { return AZ::Color(0.62f, 0.32f, 1.0f, 1.0f); }

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        AnimGraphObject::ECategory GetPaletteCategory() const override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }
        void SetWeightedNodeMask(const AZStd::vector<WeightedMaskEntry>& weightedNodeMask);
        const AZStd::vector<WeightedMaskEntry>& GetWeightedNodeMask() const { return m_weightedNodeMask; }
        void FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outBlendNodeA, AnimGraphNode** outBlendNodeB, float* outWeight, bool isAdditive, bool optimizeByWeight);

        void SetSyncMode(ESyncMode syncMode);
        ESyncMode GetSyncMode() const;

        void SetEventMode(EEventMode eventMode);
        EEventMode GetEventMode() const;

        void SetExtractionMode(EExtractionMode extractionMode);
        EExtractionMode GetExtractionMode() const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        AZStd::vector<WeightedMaskEntry>    m_weightedNodeMask;     /**< Node mask stores pairs of node name and the blend weight for the node. */
        ESyncMode                           m_syncMode;
        EEventMode                          m_eventMode;
        EExtractionMode                     m_extractionMode;

    private:
        AZStd::string GetNodeMaskNodeName(int index) const;
    };

}   // namespace EMotionFX
