/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CONTAINERS_SLIST_H
#define GM_CONTAINERS_SLIST_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/forward_list.h>

namespace GridMate
{
    template<class T, class Allocator = SysContAlloc>
    using forward_list = AZStd::forward_list<T, Allocator>;
}

#endif // GM_CONTAINERS_SLIST_H
