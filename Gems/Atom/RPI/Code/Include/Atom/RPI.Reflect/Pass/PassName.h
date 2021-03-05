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

#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        // Helper functions for building paths
        // For example if a pass has a path 'Root.Window1' and it's child pass is 'Forward'
        // we can use these concatenation functions to get 'Root.Window1.Forward'

        inline AZStd::string ConcatPassString(const AZStd::string_view& first, const AZStd::string_view& second)
        {
            return AZStd::string::format("%.*s.%.*s", static_cast<int>(first.length()), first.data(), static_cast<int>(second.length()), second.data());
        }

        inline AZStd::string ConcatPassString(const Name& first, const Name& second)
        {
            return ConcatPassString(first.GetStringView(), second.GetStringView());
        }

        inline Name ConcatPassName(const Name& first, const Name& second)
        {
            return Name(ConcatPassString(first, second));
        }
    }

}

