/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    class EMFX_API BlendTreeRagdollStrenghModifierNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRagdollStrenghModifierNode, "{9A31E551-CC6D-4FBE-BBFA-4291EAF62A86}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_POSE = 0,
            INPUTPORT_STRENGTH = 1,
            INPUTPORT_DAMPINGRATIO = 2,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_POSE = 0,
            PORTID_STRENGTH = 1,
            PORTID_DAMPINGRATIO = 2,
            PORTID_OUTPUT_POSE = 0
        };

        enum StrengthInputType : AZ::u8
        {
            STRENGTHINPUTTYPE_NONE = 0,
            STRENGTHINPUTTYPE_OVERWRITE = 1,
            STRENGTHINPUTTYPE_MULTIPLY = 2
        };

        enum DampingRatioInputType : AZ::u8
        {
            DAMPINGRATIOINPUTTYPE_NONE = 0,
            DAMPINGRATIOINPUTTYPE_OVERWRITE = 1
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            ~UniqueData() override = default;

            void Update() override;

        public:
            AZStd::vector<size_t> m_modifiedJointIndices;
        };

        BlendTreeRagdollStrenghModifierNode();
        ~BlendTreeRagdollStrenghModifierNode() override = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override                       { return AZ::Color(0.81f, 0.69f, 0.58f, 1.0f); }
        const char* GetPaletteName() const override                     { return "Ragdoll Strength Modifier"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override  { return AnimGraphObject::CATEGORY_PHYSICS; }

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        void Output(AnimGraphInstance* animGraphInstance) override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }
        bool GetHasOutputPose() const override                          { return true; }

        const AZStd::vector<AZStd::string>& GetModifiedJointNames() const { return m_modifiedJointNames; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        bool IsStrengthReadOnly() const;
        bool IsDampingRatioReadOnly() const;
        AZStd::string GetModifiedJointName(int index) const;

        AZStd::vector<AZStd::string>    m_modifiedJointNames;
        float                           m_strength;
        float                           m_dampingRatio;
        StrengthInputType               m_strengthInputType;
        DampingRatioInputType           m_dampingRatioInputType;
    };
} // namespace EMotionFX
