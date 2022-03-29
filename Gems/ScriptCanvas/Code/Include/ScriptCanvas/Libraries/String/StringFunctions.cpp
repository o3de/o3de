/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringFunctions.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvas
{
    namespace StringFunctions
    {
        AZStd::string ToLower(AZStd::string sourceString)
        {
            AZStd::to_lower(sourceString.begin(), sourceString.end());
            return sourceString;
        }

        AZStd::string ToUpper(AZStd::string sourceString)
        {
            AZStd::to_upper(sourceString.begin(), sourceString.end());
            return sourceString;
        }

        AZStd::string Substring(AZStd::string sourceString, AZ::u32 index, AZ::u32 length)
        {
            length = AZ::GetClamp<AZ::u32>(length, 0, aznumeric_cast<AZ::u32>(sourceString.size()));

            if (length == 0 || index >= sourceString.size())
            {
                return {};
            }

            return sourceString.substr(index, length);
        }
    } // namespace StringFunctions
} // namespace ScriptCanvas
