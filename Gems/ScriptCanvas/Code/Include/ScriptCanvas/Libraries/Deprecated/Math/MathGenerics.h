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
    //! MathNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace MathNodes
    {
        static constexpr const char* k_categoryName = "Math";

        AZ_INLINE Data::NumberType MultiplyAndAdd(Data::NumberType multiplicand, Data::NumberType multiplier, Data::NumberType addend)
        {
            // result = src0 * src1 + src2
            return multiplicand * multiplier + addend;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(MultiplyAndAdd, k_categoryName, "{827BDBD2-48CE-4DA4-90F3-F1B8E996613B}", "ScriptCanvas_MathFunctions_MultiplyAndAdd");

        AZ_INLINE Data::NumberType StringToNumber(const Data::StringType& stringValue)
        {
            return AZStd::stof(stringValue);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(StringToNumber, k_categoryName, "{FD2D9758-5EA2-45A3-B293-A748D951C4A3}", "ScriptCanvas_MathFunctions_StringToNumber");

        using Registrar = RegistrarGeneric<
            MultiplyAndAddNode,
            StringToNumberNode
        >;
    }
}

