/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeFloatConditionNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeFloatConditionNode, "{1FA8AD35-8730-49AB-97FD-A602728DBF22}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        //
        enum : uint16
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            OUTPUTPORT_VALUE    = 0,
            OUTPUTPORT_BOOL     = 1
        };

        enum : uint16
        {
            PORTID_INPUT_X      = 0,
            PORTID_INPUT_Y      = 1,
            PORTID_OUTPUT_VALUE = 0,
            PORTID_OUTPUT_BOOL  = 1
        };

        enum EFunction : AZ::u8
        {
            FUNCTION_EQUAL          = 0,
            FUNCTION_GREATER        = 1,
            FUNCTION_LESS           = 2,
            FUNCTION_GREATEROREQUAL = 3,
            FUNCTION_LESSOREQUAL    = 4,
            FUNCTION_NOTEQUAL       = 5,
            NUM_FUNCTIONS
        };

        enum EReturnMode : AZ::u8
        {
            MODE_VALUE  = 0,
            MODE_X      = 1,
            MODE_Y      = 2
        };

        BlendTreeFloatConditionNode();
        ~BlendTreeFloatConditionNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetFunction(EFunction func);

        AZ::Color GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetDefaultValue(float defaultValue);
        void SetTrueResult(float trueResult);
        void SetFalseResult(float falseResult);
        void SetTrueReturnMode(EReturnMode returnMode);
        void SetFalseReturnMode(EReturnMode returnMode);

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef bool (MCORE_CDECL * BlendTreeFloatConditionFunction)(float x, float y);

        EFunction                       m_functionEnum;
        BlendTreeFloatConditionFunction m_function;
        float                           m_defaultValue;
        float                           m_trueResult;
        float                           m_falseResult;
        EReturnMode                     m_trueReturnMode;
        EReturnMode                     m_falseReturnMode;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static bool MCORE_CDECL FloatConditionEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionNotEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionGreater(float x, float y);
        static bool MCORE_CDECL FloatConditionLess(float x, float y);
        static bool MCORE_CDECL FloatConditionGreaterOrEqual(float x, float y);
        static bool MCORE_CDECL FloatConditionLessOrEqual(float x, float y);
    };
} // namespace EMotionFX
