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
  
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    namespace UnitTesting
    {
        namespace Auxiliary
        {
            AZ_INLINE AZStd::vector<Data::NumberType> FillWithOrdinals(Data::NumberType count)
            {
                AZStd::vector<Data::NumberType> ordinals;
                ordinals.reserve(aznumeric_caster(count));

                for (float ordinal = 1.f; ordinal <= count; ++ordinal)
                {
                    ordinals.push_back(ordinal);
                }

                return ordinals;
            }
            SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FillWithOrdinals, "UnitTesting/Auxiliary", "{D135E6C2-DDAE-494F-B13B-F214D3E0BC20}", "", "Count");
            
            using Registrar = RegistrarGeneric
                <   FillWithOrdinalsNode
                > ;


        } // namespace Auxiliary
    } // namespace UnitTesting
} // namespace ScriptCanvas