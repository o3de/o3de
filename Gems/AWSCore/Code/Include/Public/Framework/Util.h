/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
