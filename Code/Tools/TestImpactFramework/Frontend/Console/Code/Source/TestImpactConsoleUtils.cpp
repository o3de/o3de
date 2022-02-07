/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactConsoleUtils.h>
#include <AzCore/Casting/numeric_cast.h>

namespace TestImpact
{
    namespace Console
    {
        AZStd::string SetColor(Foreground foreground, Background background)
        {
            return AZStd::string::format("\033[%u;%um", aznumeric_cast<uint32_t>(foreground), aznumeric_cast<uint32_t>(background));
        }

        AZStd::string SetColorForString(Foreground foreground, Background background, const AZStd::string& str)
        {
            return AZStd::string::format("%s%s%s", SetColor(foreground, background).c_str(), str.c_str(), ResetColor().c_str());
        }

        AZStd::string ResetColor()
        {
            return "\033[0m";
        }
    } // namespace Console 
} // namespace TestImpact
