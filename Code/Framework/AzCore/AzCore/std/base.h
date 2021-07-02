/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/config.h>
#include <initializer_list>

#define AZSTD_PRINTF printf

#if AZ_COMPILER_MSVC
#   define AZ_FORMAT_ATTRIBUTE(...)
#else
#   define AZ_FORMAT_ATTRIBUTE(STRING_INDEX, FIRST_TO_CHECK) __attribute__((__format__ (__printf__, STRING_INDEX, FIRST_TO_CHECK)))
#endif

namespace AZStd
{
    using ::size_t;
    using ::ptrdiff_t;

    using std::initializer_list;

    using std::nullptr_t;

    using sys_time_t = AZ::s64;
}
