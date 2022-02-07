/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    namespace CRCNodes
    {
        static constexpr const char* k_categoryName = "Math/Crc32";

        AZ_INLINE Data::CRCType FromString(Data::StringType value)
        {
            return AZ::Crc32(value.data());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromString, k_categoryName, "{FB95B93C-4CF6-4436-BB54-30FB48AEE7DF}", "returns a Crc32 from the string", "Value");

        AZ_INLINE Data::CRCType FromNumber(Data::NumberType value)
        {
            return AZ::Crc32(aznumeric_cast<AZ::u32>(value));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromNumber, k_categoryName, "{F3E101D7-E3A4-4FB1-B6FE-AFD34154BD02}", "returns a Crc32 from the number", "Value");

        AZ_INLINE Data::NumberType GetNumber(Data::CRCType value)
        {
            return value;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetNumber, k_categoryName, "{17A49FE2-0A34-446A-AC7F-1721B36E7908}", "returns the numeric value of a Crc32", "Value");

        using Registrar = RegistrarGeneric<
            FromStringNode
#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromNumberNode
            , GetNumberNode
#endif
        >;
    }
} 

