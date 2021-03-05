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

// include the required headers
#include "EMotionFXConfig.h"
#include "MemoryCategories.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphExitNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphExitNode, "{B589D37C-2ECD-4033-8FA9-9483BB098C60}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() = default;

            void Reset() override { m_previousNode = nullptr; }
            void Update() override;

        public:
            AnimGraphNode* m_previousNode = nullptr;
        };

        AnimGraphExitNode();
        ~AnimGraphExitNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        AZ::Color GetVisualColor() const override                   { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        bool GetCanActAsState() const override                      { return true; }
        bool GetSupportsVisualization() const override              { return true; }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        bool GetHasOutputPose() const override                      { return true; }
        bool GetIsLastInstanceDeletable() const override            { return true; }
        bool GetHasVisualOutputPorts() const override               { return false; }
        bool GetCanBeInsideChildStateMachineOnly() const override   { return true; }
        bool GetCanHaveOnlyOneInsideParent() const override         { return true; }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable) override;
        void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
