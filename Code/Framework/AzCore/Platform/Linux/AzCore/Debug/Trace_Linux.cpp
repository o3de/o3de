/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string.h>
#include <iostream>

namespace AZ::Debug::Platform
{
    void OutputToDebugger([[maybe_unused]] AZStd::string_view window, [[maybe_unused]] AZStd::string_view message)
    {
        constexpr AZStd::string_view separator = ": ";
        fwrite(window.data(), 1, window.size(), stdout);
        fwrite(separator.data(), 1, separator.size(), stdout);
        fwrite(message.data(), 1, message.size(), stdout);
    }
} // namespace AZ::Debug::Platform
