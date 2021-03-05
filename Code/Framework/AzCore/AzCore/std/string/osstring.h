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
