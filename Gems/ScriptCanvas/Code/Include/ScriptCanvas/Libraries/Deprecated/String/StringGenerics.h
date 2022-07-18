/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Math/MathUtils.h>

namespace ScriptCanvas
{
    //! StringNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace StringNodes
    {
        static constexpr const char* k_categoryName = "String";

        AZ_INLINE Data::StringType ToLower(Data::StringType sourceString)
        {
            AZStd::to_lower(sourceString.begin(), sourceString.end());
            return sourceString;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ToLower, k_categoryName, "{FC5FA07E-C65D-470A-BEFA-714EF8103866}", "ScriptCanvas_StringFunctions_ToLower");

        AZ_INLINE Data::StringType ToUpper(Data::StringType sourceString)
        {
            AZStd::to_upper(sourceString.begin(), sourceString.end());
            return sourceString;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ToUpper, k_categoryName, "{323951D4-9BB1-47C9-BD2C-2DD2B750217F}", "ScriptCanvas_StringFunctions_ToUpper");

        AZ_INLINE Data::StringType Substring(Data::StringType sourceString, AZ::u32 index, AZ::u32 length)
        {
            length = AZ::GetClamp<AZ::u32>(length, 0, aznumeric_cast<AZ::u32>(sourceString.size()));

            if (length == 0 || index >= sourceString.size())
            {
                return {};
            }

            return sourceString.substr(index, length);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Substring, k_categoryName, "{031BCDFC-5DA4-4EA0-A310-1FA9165E5BE5}", "ScriptCanvas_StringFunctions_Substring");

        using Registrar = RegistrarGeneric
            <
            ToLowerNode,
            ToUpperNode,
            SubstringNode
            >;
    }
}
