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
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{


    /**
    *
    */
    class EMFX_API BlendTreeRotationMath2Node
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeRotationMath2Node, "{7DDDBAA4-1FD5-47B0-8E34-BA27C1F52210}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_X = 0,
            INPUTPORT_Y = 1,
            OUTPUTPORT_RESULT_QUATERNION = 0
        };

        enum
        {
            PORTID_INPUT_X = 0,
            PORTID_INPUT_Y = 1,
            PORTID_OUTPUT_QUATERNION = 0
        };

        enum EMathFunction : AZ::u8
        {
            MATHFUNCTION_MULTIPLY = 0,
            MATHFUNCTION_INVERSE_MULTIPLY = 1,
            MATHFUNCTION_NUMFUNCTIONS
        };

        BlendTreeRotationMath2Node();
        ~BlendTreeRotationMath2Node();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetMathFunction(EMathFunction func);

        AZ::Color GetVisualColor() const override { return AZ::Color(0.0f, 0.48f, 0.65f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetDefaultValue(const AZ::Quaternion& value);

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef void (MCORE_CDECL * BlendTreeRotationMath2Function)(const AZ::Quaternion& inputX, const AZ::Quaternion& inputY, AZ::Quaternion* quaternionOutput);

        AZ::Quaternion                  m_defaultValue;
        EMathFunction                   m_mathFunction;
        BlendTreeRotationMath2Function  m_calculateFunc;

        void Output(AnimGraphInstance* animGraphInstance) override;

        void ExecuteMathLogic(EMotionFX::AnimGraphInstance * animGraphInstance);

        static void MCORE_CDECL CalculateMultiply(const AZ::Quaternion& inputA, const AZ::Quaternion& inputB, AZ::Quaternion* quaternionOutput);
        static void MCORE_CDECL CalculateInverseMultiply(const AZ::Quaternion& inputA, const AZ::Quaternion& inputB, AZ::Quaternion* quaternionOutput);
    };
}   // namespace EMotionFX
