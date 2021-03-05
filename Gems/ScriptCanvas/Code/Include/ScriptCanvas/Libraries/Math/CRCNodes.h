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

#include <AzCore/Math/Crc.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    namespace CrcNodes
    {
        AZ_INLINE Data::CRCType FromString(Data::StringType value)
        {
            return AZ::Crc32(value.data());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromString, "Math/Crc32", "{FB95B93C-4CF6-4436-BB54-30FB48AEE7DF}", "returns a Crc32 from the string", "Value");

        AZ_INLINE Data::CRCType FromNumber(Data::NumberType value)
        {
            return AZ::Crc32(aznumeric_cast<AZ::u32>(value));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromNumber, "Math/Crc32", "{F3E101D7-E3A4-4FB1-B6FE-AFD34154BD02}", "returns a Crc32 from the number", "Value");

        AZ_INLINE Data::NumberType GetNumber(Data::CRCType value)
        {
            return value;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetNumber, "Math/Crc32", "{17A49FE2-0A34-446A-AC7F-1721B36E7908}", "returns the numeric value of a Crc32", "Value");

        using Registrar = RegistrarGeneric<
            FromStringNode

#if ENABLE_EXTENDED_MATH_SUPPORT
            , FromNumberNode
            , GetNumberNode
#endif

        >;
    } // namespace CrcNodes
} // namespace ScriptCanvas

