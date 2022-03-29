/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace ScriptCanvas
{
    namespace StringFunctions
    {
        AZStd::string ToLower(AZStd::string sourceString);

        AZStd::string ToUpper(AZStd::string sourceString);

        AZStd::string Substring(AZStd::string sourceString, AZ::u32 index, AZ::u32 length);
    } // namespace StringFunctions
} // namespace ScriptCanvas
