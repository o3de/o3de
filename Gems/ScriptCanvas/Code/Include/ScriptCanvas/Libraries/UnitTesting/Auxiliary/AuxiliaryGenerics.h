/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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


        }
    } 
} 
