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
    namespace MathNodes
    {
        static constexpr const char* k_categoryName = "Math";

        AZ_INLINE Data::NumberType MultiplyAndAdd(Data::NumberType multiplicand, Data::NumberType multiplier, Data::NumberType addend)
        {
            // result = src0 * src1 + src2
            return multiplicand * multiplier + addend;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(MultiplyAndAdd, k_categoryName, "{827BDBD2-48CE-4DA4-90F3-F1B8E996613B}", "", "Multiplicand", "Multiplier", "Addend");

        AZ_INLINE Data::NumberType StringToNumber(const Data::StringType& stringValue)
        {
            return AZStd::stof(stringValue);
        }

        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(StringToNumber, k_categoryName, "{FD2D9758-5EA2-45A3-B293-A748D951C4A3}", "Converts the given string to it's numeric representation if possible.", "", "");

        AZ_INLINE AZStd::tuple<ScriptCanvas::Data::Vector3Type, ScriptCanvas::Data::StringType, ScriptCanvas::Data::BooleanType>
            ThreeGeneric(const ScriptCanvas::Data::Vector3Type& v, const ScriptCanvas::Data::StringType& s, const ScriptCanvas::Data::BooleanType& b)
        {
            return AZStd::make_tuple(v, s, b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(ThreeGeneric, k_categoryName, "{ABD62421-82D4-4EFB-AD99-57A8700D6402}", "returns all columns from matrix", "One", "Two", "Three", "One", "Two", "Three");

        using Registrar = RegistrarGeneric<
            ThreeGenericNode,
            MultiplyAndAddNode,
            StringToNumberNode
        >;
    }
}

