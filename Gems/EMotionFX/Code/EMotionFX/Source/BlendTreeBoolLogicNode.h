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
    class EMFX_API BlendTreeBoolLogicNode
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(BlendTreeBoolLogicNode, "{1C7C02C1-D55A-4F2B-8947-BC5163916BBA}", AnimGraphNode);
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            OUTPUTPORT_VALUE    = 0,
            OUTPUTPORT_BOOL     = 1
        };

        enum
        {
            PORTID_INPUT_X      = 0,
            PORTID_INPUT_Y      = 1,
            PORTID_OUTPUT_VALUE = 0,
            PORTID_OUTPUT_BOOL  = 1
        };

        enum EFunction : AZ::u8
        {
            FUNCTION_AND        = 0,
            FUNCTION_OR         = 1,
            FUNCTION_XOR        = 2,
            FUNCTION_NAND       = 3,
            FUNCTION_NOR        = 4,
            FUNCTION_XNOR       = 5,
            FUNCTION_NOT_X      = 6,
            FUNCTION_NOT_Y      = 7,
            NUM_FUNCTIONS
        };

        BlendTreeBoolLogicNode();
        ~BlendTreeBoolLogicNode();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void SetFunction(EFunction func);
        EFunction GetFunction() const;

        AZ::Color GetVisualColor() const override;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        void SetDefaultValue(bool defaultValue);
        void SetTrueResult(float trueResult);
        void SetFalseResult(float falseResult);

        static void Reflect(AZ::ReflectContext* context);

    private:
        typedef bool (MCORE_CDECL * BlendTreeBoolLogicFunction)(bool x, bool y);

        EFunction                   m_functionEnum;
        BlendTreeBoolLogicFunction  m_function;
        float                       m_trueResult;
        float                       m_falseResult;
        bool                        m_defaultValue;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;

        static bool MCORE_CDECL BoolLogicAND(bool x, bool y);
        static bool MCORE_CDECL BoolLogicOR(bool x, bool y);
        static bool MCORE_CDECL BoolLogicXOR(bool x, bool y);
        static bool MCORE_CDECL BoolLogicNAND(bool x, bool y);
        static bool MCORE_CDECL BoolLogicNOR(bool x, bool y);
        static bool MCORE_CDECL BoolLogicXNOR(bool x, bool y);
        static bool MCORE_CDECL BoolLogicNOTX(bool x, bool y);
        static bool MCORE_CDECL BoolLogicNOTY(bool x, bool y);
    };
} // namespace EMotionFX