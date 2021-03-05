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

#include <Libraries/Core/BinaryOperator.h>
#include <Libraries/Math/ArithmeticFunctions.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
#if defined(EVERYTHING_IS_GOING_TO_CHANGE)

            class Xor
                : public BinaryOperator<Xor, ArithmeticOperator<OperatorType::Xor>>
            {
            public:
                using BaseType = BinaryOperator<Xor, ArithmeticOperator<OperatorType::Xor>>;
                AZ_COMPONENT(Xor, "{56903089-C289-42A8-9B22-A630F4909FB6}", BaseType);

                static const char* GetOperatorName() { return "Xor"; }
                static const char* GetOperatorDesc() { return "Performs the Xor between two numbers"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Xor.png"; }

                

            };
#endif
        }
    }
}