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
#include <aws/core/utils/memory/stl/AWSString.h>

namespace AWSCore
{
    namespace Util
    {
        inline Aws::String ToAwsString(const AZStd::string& s)
        {
            return Aws::String(s.c_str(), s.length());
        }

        inline AZStd::string ToAZString(const Aws::String& s)
        {
            return AZStd::string(s.c_str(), s.length());
        }
    }

} // namespace AWSCore
