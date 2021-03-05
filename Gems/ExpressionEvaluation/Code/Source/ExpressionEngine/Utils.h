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

#include <AzCore/std/any.h>

namespace ExpressionEvaluation
{
    class Utils
    {
    public:
        template<typename ValueType>
        static ValueType GetAnyValue(const AZStd::any& operand, ValueType defaultValue = ValueType{})
        {
            if (operand.is<ValueType>())
            {
                return AZStd::any_cast<ValueType>(operand);
            }
            
            return defaultValue;
        }
    };
}