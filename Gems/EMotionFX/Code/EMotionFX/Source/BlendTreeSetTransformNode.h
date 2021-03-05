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

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/TransformSpace.h>


namespace EMotionFX
{
    class EMFX_API BlendTreeSetTransformNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeSetTransformNode, "{2AFA0051-C4B0-403D-95F2-55F85E1542A7}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        // input ports
        enum
        {
            INPUTPORT_POSE              = 0,
            INPUTPORT_TRANSLATION       = 1,
            INPUTPORT_ROTATION          = 2,
            INPUTPORT_SCALE             = 3
        };

        enum
        {
            PORTID_INPUT_POSE               = 0,
            PORTID_INPUT_TRANSLATION        = 1,
            PORTID_INPUT_ROTATION           = 2,
            PORTID_INPUT_SCALE              = 3
        };

        // output ports
        enum
        {
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE = 0
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
            uint32 m_nodeIndex = InvalidIndex32;
        };

        BlendTreeSetTransformNode();
        ~BlendTreeSetTransformNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        AZ::Color GetVisualColor() const override               { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }
        bool GetSupportsDisable() const override                { return true; }
        bool GetSupportsVisualization() const override          { return true; }
        bool GetHasOutputPose() const override                  { return true; }
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override     { return GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue(); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetJointName(const AZStd::string& jointName) { m_nodeName = jointName; }
        const AZStd::string& GetJointName() { return m_nodeName; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;

        AZStd::string       m_nodeName;
        ETransformSpace     m_transformSpace;
    };
}   // namespace EMotionFX
