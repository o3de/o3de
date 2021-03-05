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
    class EMFX_API BlendTreeFloatMath1Node
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFloatMath1Node, "{F6B9FDF5-6192-4CB7-A18D-447C0363C041}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_X         = 0,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_OUTPUT_RESULT    = 1
        };

        enum EMathFunction : AZ::u8
        {
            MATHFUNCTION_SIN            = 0,
            MATHFUNCTION_COS            = 1,
            MATHFUNCTION_TAN            = 2,
            MATHFUNCTION_SQR            = 3,
            MATHFUNCTION_SQRT           = 4,
            MATHFUNCTION_ABS            = 5,
            MATHFUNCTION_FLOOR          = 6,
            MATHFUNCTION_CEIL           = 7,
            MATHFUNCTION_ONEOVERINPUT   = 8,
            MATHFUNCTION_INVSQRT        = 9,
            MATHFUNCTION_LOG            = 10,
            MATHFUNCTION_LOG10          = 11,
            MATHFUNCTION_EXP            = 12,
            MATHFUNCTION_FRACTION       = 13,
            MATHFUNCTION_SIGN           = 14,
            MATHFUNCTION_ISPOSITIVE     = 15,
            MATHFUNCTION_ISNEGATIVE     = 16,
            MATHFUNCTION_ISNEARZERO     = 17,
            MATHFUNCTION_RANDOMFLOAT    = 18,
            MATHFUNCTION_RADTODEG       = 19,
            MATHFUNCTION_DEGTORAD       = 20,
            MATHFUNCTION_SMOOTHSTEP     = 21,
            MATHFUNCTION_ACOS           = 22,
            MATHFUNCTION_ASIN           = 23,
            MATHFUNCTION_ATAN           = 24,
            MATHFUNCTION_NEGATE         = 25,
            MATHFUNCTION_NUMFUNCTIONS
        };

        BlendTreeFloatMath1Node();
        ~BlendTreeFloatMath1Node();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetMathFunction(EMathFunction func);

        AZ::Color GetVisualColor() const override;
        bool GetSupportsDisable() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef float (MCORE_CDECL * BlendTreeMath1Function)(float input);

        EMathFunction           m_mathFunction;
        BlendTreeMath1Function  m_calculateFunc;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static float MCORE_CDECL CalculateSin(float input);
        static float MCORE_CDECL CalculateCos(float input);
        static float MCORE_CDECL CalculateTan(float input);
        static float MCORE_CDECL CalculateSqr(float input);
        static float MCORE_CDECL CalculateSqrt(float input);
        static float MCORE_CDECL CalculateAbs(float input);
        static float MCORE_CDECL CalculateFloor(float input);
        static float MCORE_CDECL CalculateCeil(float input);
        static float MCORE_CDECL CalculateOneOverInput(float input);
        static float MCORE_CDECL CalculateInvSqrt(float input);
        static float MCORE_CDECL CalculateLog(float input);
        static float MCORE_CDECL CalculateLog10(float input);
        static float MCORE_CDECL CalculateExp(float input);
        static float MCORE_CDECL CalculateFraction(float input);
        static float MCORE_CDECL CalculateSign(float input);
        static float MCORE_CDECL CalculateIsPositive(float input);
        static float MCORE_CDECL CalculateIsNegative(float input);
        static float MCORE_CDECL CalculateIsNearZero(float input);
        static float MCORE_CDECL CalculateRandomFloat(float input);
        static float MCORE_CDECL CalculateRadToDeg(float input);
        static float MCORE_CDECL CalculateDegToRad(float input);
        static float MCORE_CDECL CalculateSmoothStep(float input);
        static float MCORE_CDECL CalculateACos(float input);
        static float MCORE_CDECL CalculateASin(float input);
        static float MCORE_CDECL CalculateATan(float input);
        static float MCORE_CDECL CalculateNegate(float input);
    };
}   // namespace EMotionFX
