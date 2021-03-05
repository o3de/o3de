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
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeVector3Math2Node
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeVector3Math2Node, "{30568371-DEDE-47CC-95C2-7EB7A353264B}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_X                 = 0,
            INPUTPORT_Y                 = 1,
            OUTPUTPORT_RESULT_VECTOR3   = 0,
            OUTPUTPORT_RESULT_FLOAT     = 1
        };

        enum
        {
            PORTID_INPUT_X              = 0,
            PORTID_INPUT_Y              = 1,
            PORTID_OUTPUT_VECTOR3       = 0,
            PORTID_OUTPUT_FLOAT         = 1
        };

        enum EMathFunction : AZ::u8
        {
            MATHFUNCTION_DOT            = 0,
            MATHFUNCTION_CROSS          = 1,
            MATHFUNCTION_ADD            = 2,
            MATHFUNCTION_SUBTRACT       = 3,
            MATHFUNCTION_MULTIPLY       = 4,
            MATHFUNCTION_DIVIDE         = 5,
            MATHFUNCTION_ANGLEDEGREES   = 6,
            MATHFUNCTION_NUMFUNCTIONS
        };

        BlendTreeVector3Math2Node();
        ~BlendTreeVector3Math2Node();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetMathFunction(EMathFunction func);

        AZ::Color GetVisualColor() const override      { return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetDefaultValue(const AZ::Vector3& value);

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef void (MCORE_CDECL * BlendTreeVec3Math1Function)(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);

        AZ::Vector3                 m_defaultValue;
        EMathFunction               m_mathFunction;
        BlendTreeVec3Math1Function  m_calculateFunc;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static void MCORE_CDECL CalculateDot(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateCross(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAdd(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateSubtract(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateMultiply(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateDivide(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAngleDegrees(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput);
    };
}   // namespace EMotionFX
