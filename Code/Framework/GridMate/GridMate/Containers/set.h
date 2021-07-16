/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CONTAINERS_SET_H
#define GM_CONTAINERS_SET_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/set.h>

namespace GridMate
{
    template<class Key, class Compare = AZStd::less<Key>, class Allocator = SysContAlloc>
    using set = AZStd::set<Key, Compare, Allocator>;

    template<class Key, class Compare = AZStd::less<Key>, class Allocator = SysContAlloc>
    using multiset = AZStd::multiset<Key, Compare, Allocator>;
}

#endif // GM_CONTAINERS_SET_H
