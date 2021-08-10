/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    class EMFX_API BlendTreeGetTransformNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeGetTransformNode, "{97E4FC12-7468-4F06-AF93-2FE2472E2B59}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        // input ports
        enum
        {
            INPUTPORT_POSE                  = 0
        };

        enum
        {
            PORTID_INPUT_POSE               = 0
        };

        // output ports
        enum
        {
            OUTPUTPORT_TRANSLATION          = 0,
            OUTPUTPORT_ROTATION             = 1,
            OUTPUTPORT_SCALE                = 2
        };

        enum
        {
            PORTID_OUTPUT_TRANSLATION      = 0,
            PORTID_OUTPUT_ROTATION         = 1,
            PORTID_OUTPUT_SCALE            = 2
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
            size_t m_nodeIndex = InvalidIndex;
        };

        BlendTreeGetTransformNode();
        ~BlendTreeGetTransformNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        AZ::Color GetVisualColor() const override                  { return AZ::Color(1.0f, 0.0f, 0.0f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetJointName(const AZStd::string& jointName) { m_actorNode.first = jointName; }
        const AZStd::string& GetNodeName() const { return m_actorNode.first; }
        int GetActorInstanceParentDepth() const { return m_actorNode.second; }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Output(AnimGraphInstance* animGraphInstance) override;

        using ActorNode = AZStd::pair<AZStd::string, int>;
        ActorNode           m_actorNode;
        ETransformSpace     m_transformSpace;
    };
}   // namespace EMotionFX
