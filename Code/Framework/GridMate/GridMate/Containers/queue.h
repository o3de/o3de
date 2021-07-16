/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_CONTAINERS_QUEUE_H
#define GM_CONTAINERS_QUEUE_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/queue.h>

namespace GridMate
{
    template<class T, class Container = AZStd::deque<T, SysContAlloc> >
    using queue = AZStd::queue<T, Container>;
}

#endif // GM_CONTAINERS_QUEUE_H
