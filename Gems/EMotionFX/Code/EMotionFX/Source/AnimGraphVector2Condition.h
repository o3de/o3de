/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphVector2Condition 
        : public AnimGraphTransitionCondition
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(AnimGraphVector2Condition, "{605DF8B0-C39A-4BB4-B1A9-ABAF528E0739}", AnimGraphTransitionCondition, ObjectAffectedByParameterChanges)
        AZ_CLASS_ALLOCATOR_DECL

        enum EOperation : AZ::u8
        {
            OPERATION_LENGTH = 0,
            OPERATION_GETX = 1,
            OPERATION_GETY = 2
        };

        AnimGraphVector2Condition();
        AnimGraphVector2Condition(AnimGraph* animGraph);
        ~AnimGraphVector2Condition();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        void SetFunction(AnimGraphParameterCondition::EFunction func);
        const char* GetTestFunctionString() const;

        void SetOperation(EOperation operation);
        const char* GetOperationString() const;

        AZ::TypeId GetParameterType() const;
        AZ::Outcome<size_t> GetParameterIndex() const;
        void SetParameterName(const AZStd::string& parameterName);
        void SetTestValue(float testValue);
        void SetRangeValue(float rangeValue);

        // ObjectAffectedByParameterChanges overrides
        void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        void ParameterRemoved(const AZStd::string& oldParameterName) override;
        void BuildParameterRemovedCommands(MCore::CommandGroup& commandGroup, const AZStd::string& parameterNameToBeRemoved) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        // test function types
        typedef bool (MCORE_CDECL * BlendConditionParamValueFunction)(float paramValue, float testValue, float rangeValue);
        typedef float (MCORE_CDECL * BlendConditionOperationFunction)(const AZ::Vector2& vec);

        // float test functions
        static bool MCORE_CDECL TestGreater(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestGreaterEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLess(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestLessEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotEqual(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestInRange(float paramValue, float testValue, float rangeValue);
        static bool MCORE_CDECL TestNotInRange(float paramValue, float testValue, float rangeValue);

        // operation functions
        static float MCORE_CDECL OperationLength(const AZ::Vector2& vec);
        static float MCORE_CDECL OperationGetX(const AZ::Vector2& vec);
        static float MCORE_CDECL OperationGetY(const AZ::Vector2& vec);

        AZ::Crc32 GetRangeValueVisibility() const;

        static const char* s_operationLength;
        static const char* s_operationGetX;
        static const char* s_operationGetY;

        AZStd::string                           m_parameterName;
        AZ::Outcome<size_t>                     m_parameterIndex;
        EOperation                              m_operation;
        BlendConditionOperationFunction         m_operationFunction;
        AnimGraphParameterCondition::EFunction  m_function;
        BlendConditionParamValueFunction        m_testFunction;
        float                                   m_testValue;
        float                                   m_rangeValue;
    };
} // namespace EMotionFX
