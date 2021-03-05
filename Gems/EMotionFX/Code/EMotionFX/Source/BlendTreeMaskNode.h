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

#include <AzCore/std/optional.h>
#include <EMotionFX/Source/ActorBus.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    class EMFX_API BlendTreeMaskNode
        : public AnimGraphNode
        , public ActorNotificationBus::Handler
    {
    public:
        AZ_RTTI(BlendTreeMaskNode, "{EC50F91C-8BB1-4D49-B13E-F639D2505DB7}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_BASEPOSE  = 0,
            INPUTPORT_START     = 1, //INPUTPORT_POSE1..N= INPUTPORT_START+i
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_RESULT    = 0
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
            struct MaskInstance
            {
                AZ::u32 m_inputPortNr;
                AZStd::vector<AZ::u32> m_jointIndices;
            };

            AZStd::vector<MaskInstance> m_maskInstances;
            AZStd::optional<AZ::u32> m_motionExtractionInputPortNr;
        };

        BlendTreeMaskNode();
        ~BlendTreeMaskNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        // ActorNotificationBus overrides
        void OnMotionExtractionNodeChanged(Actor* actor, Node* newMotionExtractionNode) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        bool GetHasOutputPose() const override { return true; }
        bool GetSupportsVisualization() const override { return true; }
        AZ::Color GetVisualColor() const override { return AZ::Color(0.2f, 0.78f, 0.2f, 1.0f); }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override         { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override { return "Pose Mask";}
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_BLENDING; }

        bool GetOutputEvents(size_t inputPortNr) const;

        void SetMask(size_t maskIndex, const AZStd::vector<AZStd::string>& jointNames);
        void SetOutputEvents(size_t maskIndex, bool outputEvents);

        static void Reflect(AZ::ReflectContext* context);

        class Mask
        {
        public:
            AZ_RTTI(BlendTreeMaskNode::Mask, "{74750F38-24B3-465B-9CA1-740ACF947DC1}")
            AZ_CLASS_ALLOCATOR_DECL

            virtual ~Mask() = default;
            
            static void Reflect(AZ::ReflectContext* context);
            void Reinit();
            AZStd::string GetMaskName() const;
            AZStd::string GetOutputEventsName() const;

            AZStd::vector<AZStd::string> m_jointNames;
            bool m_outputEvents = true;

            AZ::u8 m_maskIndex = 0;
            BlendTreeMaskNode* m_parent = nullptr;
        };

        const AZStd::vector<Mask>& GetMasks() const { return m_masks; }

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        size_t GetNumUsedMasks() const;

        AZStd::string GetMaskJointName(size_t maskIndex, size_t jointIndex) const;

        AZStd::vector<Mask> m_masks;
        static const size_t s_numMasks;
    };
} // namespace EMotionFX
