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

#include <AzFramework/Physics/Ragdoll.h>
#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    class EMFX_API BlendTreeRagdollNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRagdollNode, "{DB81AD7E-15D4-4563-AD9D-B14A7BBB22DB}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_TARGETPOSE = 0,
            INPUTPORT_ACTIVATE = 1,
            OUTPUTPORT_POSE = 0
        };

        enum
        {
            PORTID_TARGETPOSE = 0,
            PORTID_ACTIVATE = 1,
            PORTID_OUTPUT_POSE = 0
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
            AZStd::vector<AZ::u8> m_simulatedJointStates; /** Flags indicating if the joint at the given index in the animation skeleton is added to the physics simulation by this node. Size is Skeleton::GetNumNodes(). */
            bool m_isRagdollRootNodeSimulated = false;
        };

        BlendTreeRagdollNode();
        ~BlendTreeRagdollNode() override = default;

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AZ::Color GetVisualColor() const override                       { return AZ::Color(0.81f, 0.69f, 0.58f, 1.0f); }
        const char* GetPaletteName() const override                     { return "Activate Ragdoll Joints"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override  { return AnimGraphObject::CATEGORY_PHYSICS; }

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }
        bool GetHasOutputPose() const override                          { return true; }

        void SetSimulatedJointNames(const AZStd::vector<AZStd::string>& simulatedJointNames) { m_simulatedJointNames = simulatedJointNames; }
        AZStd::vector<AZStd::string>& GetSimulatedJointNames()          { return m_simulatedJointNames; }
        bool IsActivated(AnimGraphInstance* animGraphInstance) const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string GetSimulatedJointName(int index) const;

        AZStd::vector<AZStd::string> m_simulatedJointNames;
    };
} // namespace EMotionFX
