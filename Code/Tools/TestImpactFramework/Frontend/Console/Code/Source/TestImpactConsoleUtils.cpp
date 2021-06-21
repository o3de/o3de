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

#include <TestImpactConsoleUtils.h>

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
