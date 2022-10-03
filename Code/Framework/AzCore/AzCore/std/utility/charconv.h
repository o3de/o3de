/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <charconv>

namespace AZStd
{
    // Bring in std::errc as well from the <system_error> header
    using std::errc;

    // libc++ still doesn't fully support std::from_chars for floating values
#if __cpp_lib_to_chars >= 201611L
    using std::chars_format;
#endif
    using std::to_chars;
    using std::to_chars_result;
    using std::from_chars;
    using std::from_chars_result;
}
