/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphNode.h>


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeFloatConstantNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFloatConstantNode, "{033D3D2F-04D3-439F-BFC3-1BDE16BBE37A}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_RESULT = 0
        };

        enum
        {
            PORTID_OUTPUT_RESULT = 0
        };

        BlendTreeFloatConstantNode();
        ~BlendTreeFloatConstantNode();

        bool InitAfterLoading(AnimGraph* animGraph) override;
        void Reinit() override;

        AZ::Color GetVisualColor() const override;
        bool GetSupportsDisable() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetValue(float value);
        static void Reflect(AZ::ReflectContext* context);

    private:
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        float   m_value;
    };

}   // namespace EMotionFX
