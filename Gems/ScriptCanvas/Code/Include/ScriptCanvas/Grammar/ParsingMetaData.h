/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Primitives.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        MetaDataPtr CreateMetaData(ExecutionTreePtr execution);
                
        struct ForEachMetaData
            : public MetaData
        {
            AZ_RTTI(ForEachMetaData, "{5610DD68-50EE-47AE-97F3-E47F73C2741E}", MetaData);
            AZ_CLASS_ALLOCATOR(ForEachMetaData, AZ::SystemAllocator);

            bool m_isKeyRequired = false;

            AZStd::string m_iteratorVariableName;
            AZStd::string m_valueFunctionVariableName;
            AZStd::string m_keyFunctionVariableName;
            AZStd::string m_nextFunctionVariableName;
            AZStd::string m_isNotAtEndFunctionVariableName;
        };

        struct FormatStringMetaData
            : public MetaData
        {
            AZ_RTTI(FormatStringMetaData, "{5FD2ED4E-5B90-42FD-9F1C-D20CA107FC97}", MetaData);
            AZ_CLASS_ALLOCATOR(FormatStringMetaData, AZ::SystemAllocator);

            void PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr execution) override;
        };

        struct FunctionCallDefaultMetaData
            : public MetaData
        {
            AZ_RTTI(FunctionCallDefaultMetaData, "{2C8D68DB-35D3-4ACA-BCE6-8498E744DEB2}", MetaData);
            AZ_CLASS_ALLOCATOR(FunctionCallDefaultMetaData, AZ::SystemAllocator);

            AZ::TypeId multiReturnType;

            void PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr execution) override;
        };

        struct MathExpressionMetaData
            : public MetaData
        {
            AZ_RTTI(MathExpressionMetaData, "{233D4756-BF46-4699-B21D-A16EEB896D8B}", MetaData);
            AZ_CLASS_ALLOCATOR(MathExpressionMetaData, AZ::SystemAllocator);

            AZStd::string m_expressionString;

            void PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr execution) override;
        };

        struct PrintMetaData
            : public MetaData
        {
            AZ_RTTI(PrintMetaData, "{41184DB3-E2E3-4621-A93F-27A2600CA294}", MetaData);
            AZ_CLASS_ALLOCATOR(PrintMetaData, AZ::SystemAllocator);

            void PostParseExecutionTreeBody(AbstractCodeModel& model, ExecutionTreePtr execution) override;
        };

        class UserFunctionNodeCallMetaData
        {
        public:
            AZ_TYPE_INFO(UserFunctionNodeCallMetaData, "{893A827F-9340-4FE1-9829-69E2602F37A1}");
            AZ_CLASS_ALLOCATOR(UserFunctionNodeCallMetaData, AZ::SystemAllocator);

            bool m_isLocal = false;
        };
    } 

} 
