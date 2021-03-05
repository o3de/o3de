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
#ifndef GM_CONTAINERS_UNORDERED_MAP_H
#define GM_CONTAINERS_UNORDERED_MAP_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/unordered_map.h>

namespace GridMate
{
    template<class Key, class MappedType, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = SysContAlloc>
    using unordered_map = AZStd::unordered_map<Key, MappedType, Hasher, EqualKey, Allocator>;

    template<class Key, class MappedType, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = SysContAlloc>
    using unordered_multimap = AZStd::unordered_multimap<Key, MappedType, Hasher, EqualKey, Allocator>;
}
#endif // GM_CONTAINERS_UNORDERED_MAP_H
