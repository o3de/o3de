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

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    namespace Console
    {
        //! The set of available foreground colors.
        enum class Foreground
        {
            Black = 30,
            Red,
            Green,
            Yellow,
            Blue,
            Magenta,
            Cyan,
            White
        };

        //! The set of available background colors.
        enum class Background
        {
            Black = 40,
            Red,
            Green,
            Yellow,
            Blue,
            Magenta,
            Cyan,
            White
        };

        //! Returns a string to be used to set the specified foreground and background color.
        AZStd::string SetColor(Foreground foreground, Background background);

        //! Returns a string with the specified string set to the specified foreground and background color followed by a color reset.
        AZStd::string SetColorForString(Foreground foreground, Background background, const AZStd::string& str);

        //! Returns a string to be used to reset the color back to white foreground on black background. 
        AZStd::string ResetColor();
    } // namespace Console 
} // namespace TestImpact
