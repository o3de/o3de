/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimGraphNode.h"
#include "EMotionFXConfig.h"

namespace MCore
{
    class Attribute;
}

namespace EMotionFX
{
    //! A compact pass-through node whose port type is fixed when it is inserted on a connection.
    class EMFX_API BlendTreeRerouteNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRerouteNode, "{1D123846-ABEF-4F52-8E7D-5C2E760B74E3}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum : AZ::u16
        {
            INPUTPORT_VALUE = 0,
            OUTPUTPORT_VALUE = 0
        };

        enum : AZ::u32
        {
            PORTID_INPUT_VALUE = 0,
            PORTID_OUTPUT_VALUE = 1
        };

        BlendTreeRerouteNode();
        ~BlendTreeRerouteNode() override;

        static void Reflect(AZ::ReflectContext* context);

        bool InitAfterLoading(AnimGraph* animGraph) override;
        void Reinit() override;

        const char* GetPaletteName() const override { return "Reroute"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_MISC; }
        AZ::Color GetVisualColor() const override { return AZ::Color(0.35f, 0.35f, 0.35f, 1.0f); }
        bool GetHasOutputPose() const override;
        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override;

        void SetDataTypeId(AZ::u32 dataTypeId) { m_dataTypeId = dataTypeId; }
        AZ::u32 GetDataTypeId() const { return m_dataTypeId; }

    private:
        void ConfigurePorts();
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;

        AZ::u32 m_dataTypeId;
        MCore::Attribute* m_defaultValue = nullptr;
    };
} // namespace EMotionFX
