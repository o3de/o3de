/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CONTAINERS_UNORDERED_SET_H
#define GM_CONTAINERS_UNORDERED_SET_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/unordered_set.h>

namespace GridMate
{
    template<class Key, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = SysContAlloc>
    using unordered_set = AZStd::unordered_set<Key, Hasher, EqualKey, Allocator>;

    template<class Key, class Hasher = AZStd::hash<Key>, class EqualKey = AZStd::equal_to<Key>, class Allocator = SysContAlloc>
    using unordered_multiset = AZStd::unordered_multiset<Key, Hasher, EqualKey, Allocator>;
}

#endif // GM_CONTAINERS_UNORDERED_SET_H
