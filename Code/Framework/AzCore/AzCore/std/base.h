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
