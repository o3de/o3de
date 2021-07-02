/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_OS_STRING_H
#define AZCORE_OS_STRING_H

#include <AzCore/std/string/string.h>
#include <AzCore/Memory/OSAllocator.h>

namespace AZ
{
    // ASCII string type that allocates from the OS allocator rather than the system allocator.
    // This should be used very sparingly, and only in application bootstrap situations where
    // the system allocator is not yet available.
    typedef AZStd::basic_string<char, AZStd::char_traits<char>, OSStdAllocator> OSString;
} // namespace AZ

#endif // AZCORE_OS_STRING_H
