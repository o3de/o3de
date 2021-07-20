/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
