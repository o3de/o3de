/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    template struct AzTypeInfo<AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::allocator>, false>;
    namespace Internal
    {
        template struct AggregateTypes<Crc32>;
    }
}

