/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
