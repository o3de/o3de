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
#ifndef GM_CONTAINERS_VECTOR_H
#define GM_CONTAINERS_VECTOR_H

#include <GridMate/Memory.h>
#include <AzCore/std/containers/vector.h>

namespace GridMate
{
    template<class T, class Allocator = SysContAlloc>
    using vector = AZStd::vector<T, Allocator>;
}
#endif // GM_CONTAINERS_VECTOR_H
