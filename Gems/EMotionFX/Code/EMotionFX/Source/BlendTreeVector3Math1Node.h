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

#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeVector3Math1Node
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeVector3Math1Node, "{79488BAA-7151-4B49-B4EB-0FCA268EF44F}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_X                 = 0,
            OUTPUTPORT_RESULT_VECTOR3   = 0,
            OUTPUTPORT_RESULT_FLOAT     = 1
        };

        enum
        {
            PORTID_INPUT_X              = 0,
            PORTID_OUTPUT_VECTOR3       = 0,
            PORTID_OUTPUT_FLOAT         = 1
        };

        enum EMathFunction : AZ::u8
        {
            MATHFUNCTION_LENGTH         = 0,
            MATHFUNCTION_SQUARELENGTH   = 1,
            MATHFUNCTION_NORMALIZE      = 2,
            MATHFUNCTION_ZERO           = 3,
            MATHFUNCTION_FLOOR          = 4,
            MATHFUNCTION_CEIL           = 5,
            MATHFUNCTION_ABS            = 6,
            MATHFUNCTION_RANDOM         = 7,
            MATHFUNCTION_RANDOMNEG      = 8,
            MATHFUNCTION_RANDOMDIRVEC   = 9,
            MATHFUNCTION_NEGATE         = 10,
            MATHFUNCTION_NUMFUNCTIONS
        };

        BlendTreeVector3Math1Node();
        ~BlendTreeVector3Math1Node();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;
        
        void SetMathFunction(EMathFunction func);

        AZ::Color GetVisualColor() const override  { return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef void (MCORE_CDECL * BlendTreeVec3Math1Function)(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);

        EMathFunction               m_mathFunction;
        BlendTreeVec3Math1Function  m_calculateFunc;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static void MCORE_CDECL CalculateLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateSquareLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateNormalize(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateZero(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateAbs(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateFloor(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateCeil(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVector(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVectorNeg(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateRandomVectorDir(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
        static void MCORE_CDECL CalculateNegate(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput);
    };
}   // namespace EMotionFX
