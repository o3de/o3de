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
    class EMFX_API BlendTreeFloatMath2Node
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFloatMath2Node, "{9F5FA0EE-B6EE-420C-9015-26E5F17AAA3E}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            OUTPUTPORT_RESULT   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_INPUT_Y          = 1,
            PORTID_OUTPUT_RESULT    = 0
        };

        enum EMathFunction : AZ::u8
        {
            MATHFUNCTION_ADD            = 0,
            MATHFUNCTION_SUBTRACT       = 1,
            MATHFUNCTION_MULTIPLY       = 2,
            MATHFUNCTION_DIVIDE         = 3,
            MATHFUNCTION_AVERAGE        = 4,
            MATHFUNCTION_RANDOMFLOAT    = 5,
            MATHFUNCTION_MOD            = 6,
            MATHFUNCTION_MIN            = 7,
            MATHFUNCTION_MAX            = 8,
            MATHFUNCTION_POW            = 9,
            MATHFUNCTION_NUMFUNCTIONS
        };

        BlendTreeFloatMath2Node();
        ~BlendTreeFloatMath2Node();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetMathFunction(EMathFunction func);

        AZ::Color GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetDefaultValue(float value);

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef float (MCORE_CDECL * BlendTreeMath2Function)(float x, float y);

        BlendTreeMath2Function  m_calculateFunc;
        EMathFunction           m_mathFunction;
        float                   m_defaultValue;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static float MCORE_CDECL CalculateAdd(float x, float y);
        static float MCORE_CDECL CalculateSubtract(float x, float y);
        static float MCORE_CDECL CalculateMultiply(float x, float y);
        static float MCORE_CDECL CalculateDivide(float x, float y);
        static float MCORE_CDECL CalculateAverage(float x, float y);
        static float MCORE_CDECL CalculateRandomFloat(float x, float y);
        static float MCORE_CDECL CalculateMod(float x, float y);
        static float MCORE_CDECL CalculateMin(float x, float y);
        static float MCORE_CDECL CalculateMax(float x, float y);
        static float MCORE_CDECL CalculatePow(float x, float y);
    };
}   // namespace EMotionFX
